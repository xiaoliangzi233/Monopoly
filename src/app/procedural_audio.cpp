#include "app/procedural_audio.h"

#include <QSettings>
#include <QtMath>

#ifdef NEON_HAVE_MULTIMEDIA
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QMutex>
#include <array>
#include <cmath>
#endif

namespace neon {

class ProceduralAudio::Backend {
public:
    virtual ~Backend() = default;
    virtual bool available() const = 0;
    virtual void configure(qreal, qreal, bool, int, int) = 0;
    virtual void trigger(int) = 0;
};

#ifdef NEON_HAVE_MULTIMEDIA
namespace {

constexpr double Pi = 3.14159265358979323846;

class SynthDevice final : public QIODevice {
public:
    SynthDevice() { open(QIODevice::ReadOnly); }
    qint64 readData(char *data, qint64 maxSize) override
    {
        auto *samples = reinterpret_cast<qint16 *>(data);
        const qint64 frames = maxSize / (2 * qint64(sizeof(qint16)));
        QMutexLocker lock(&mutex);
        constexpr double sampleRate = 48000.0;
        constexpr std::array<int, 10> pentatonic{0, 2, 4, 7, 9, 12, 14, 16, 19, 21};
        for (qint64 frame = 0; frame < frames; ++frame, ++sampleIndex) {
            const double seconds = sampleIndex / sampleRate;
            const double tempo = mode == 1 ? 1.65 : (mode == 2 ? 1.2 : .82);
            const double beat = seconds * tempo;
            const int step = int(std::floor(beat * 2.0)) % pentatonic.size();
            const double phaseInStep = std::fmod(beat * 2.0, 1.0);
            const double base = 146.83 * std::pow(2.0, district / 24.0);
            const double frequency = base * std::pow(2.0, pentatonic[step] / 12.0);
            const double pluck = std::sin(2.0 * Pi * frequency * seconds)
                * std::exp(-phaseInStep * 7.5) * .25;
            const double overtone = std::sin(2.0 * Pi * frequency * 2.01 * seconds)
                * std::exp(-phaseInStep * 11.0) * .08;
            const double flute = std::sin(2.0 * Pi * (base * .5) * seconds + std::sin(seconds * 3.0) * .02) * .045;
            double effectMix = 0.0;
            for (auto &voice : voices) {
                if (voice.remaining <= 0) continue;
                const double age = (voice.length - voice.remaining) / sampleRate;
                const double envelope = std::exp(-age * (5.0 + voice.id % 5));
                const double effectFrequency = 220.0 + (voice.id % 12) * 32.0;
                effectMix += std::sin(2.0 * Pi * effectFrequency * age) * envelope * .3;
                --voice.remaining;
            }
            const double mixed = muted ? 0.0 : (pluck + overtone + flute) * music + effectMix * effects;
            const qint16 value = qint16(qBound(-1.0, mixed, 1.0) * 26000.0);
            samples[frame * 2] = value;
            samples[frame * 2 + 1] = value;
        }
        return frames * 2 * qint64(sizeof(qint16));
    }
    qint64 writeData(const char *, qint64) override { return -1; }
    void configure(qreal musicValue, qreal effectsValue, bool muteValue, int districtValue, int modeValue)
    {
        QMutexLocker lock(&mutex);
        music = musicValue; effects = effectsValue; muted = muteValue;
        district = qBound(0, districtValue, 7); mode = qBound(0, modeValue, 2);
    }
    void trigger(int id)
    {
        QMutexLocker lock(&mutex);
        auto it = std::min_element(voices.begin(), voices.end(), [](const auto &a, const auto &b) {
            return a.remaining < b.remaining;
        });
        it->id = ((id % 24) + 24) % 24;
        it->length = 4800 + (it->id % 6) * 900;
        it->remaining = it->length;
    }
private:
    struct Voice { int id = 0; int length = 0; int remaining = 0; };
    QMutex mutex;
    std::array<Voice, 8> voices{};
    quint64 sampleIndex = 0;
    qreal music = .42;
    qreal effects = .72;
    bool muted = false;
    int district = 0;
    int mode = 0;
};

class MultimediaBackend final : public ProceduralAudio::Backend {
public:
    MultimediaBackend()
    {
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        const auto deviceInfo = QMediaDevices::defaultAudioOutput();
        if (!deviceInfo.isNull() && deviceInfo.isFormatSupported(format)) {
            sink = std::make_unique<QAudioSink>(deviceInfo, format);
            sink->setBufferSize(48000);
            sink->start(&device);
        }
    }
    bool available() const override { return bool(sink); }
    void configure(qreal music, qreal effects, bool muted, int district, int mode) override
    { device.configure(music, effects, muted, district, mode); }
    void trigger(int id) override { device.trigger(id); }
private:
    SynthDevice device;
    std::unique_ptr<QAudioSink> sink;
};

} // namespace
#else
namespace {
class SilentBackend final : public ProceduralAudio::Backend {
public:
    bool available() const override { return false; }
    void configure(qreal, qreal, bool, int, int) override {}
    void trigger(int) override {}
};
} // namespace
#endif

ProceduralAudio::ProceduralAudio(QObject *parent) : QObject(parent)
{
    QSettings settings;
    m_musicVolume = settings.value(QStringLiteral("audio/music"), .42).toReal();
    m_effectsVolume = settings.value(QStringLiteral("audio/effects"), .72).toReal();
    m_muted = settings.value(QStringLiteral("audio/muted"), false).toBool();
#ifdef NEON_HAVE_MULTIMEDIA
    m_backend = std::make_unique<MultimediaBackend>();
#else
    m_backend = std::make_unique<SilentBackend>();
#endif
    apply();
}

ProceduralAudio::~ProceduralAudio() = default;
bool ProceduralAudio::available() const { return m_backend && m_backend->available(); }

void ProceduralAudio::apply()
{
    if (m_backend) m_backend->configure(m_musicVolume, m_effectsVolume, m_muted, m_district, m_contextMode);
}

void ProceduralAudio::setMusicVolume(qreal value)
{
    value = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(value, m_musicVolume)) return;
    m_musicVolume = value; QSettings().setValue(QStringLiteral("audio/music"), value); apply(); emit settingsChanged();
}

void ProceduralAudio::setEffectsVolume(qreal value)
{
    value = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(value, m_effectsVolume)) return;
    m_effectsVolume = value; QSettings().setValue(QStringLiteral("audio/effects"), value); apply(); emit settingsChanged();
}

void ProceduralAudio::setMuted(bool value)
{
    if (m_muted == value) return;
    m_muted = value; QSettings().setValue(QStringLiteral("audio/muted"), value); apply(); emit settingsChanged();
}

void ProceduralAudio::setDistrict(int value)
{
    value = qBound(0, value, 7);
    if (m_district == value) return;
    m_district = value; apply(); emit contextChanged();
}

void ProceduralAudio::setContextMode(int value)
{
    value = qBound(0, value, 2);
    if (m_contextMode == value) return;
    m_contextMode = value; apply(); emit contextChanged();
}

void ProceduralAudio::playEffect(int effectId)
{
    if (m_backend) m_backend->trigger(effectId);
}

} // namespace neon
