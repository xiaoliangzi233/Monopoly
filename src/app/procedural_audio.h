#pragma once

#include <QObject>
#include <memory>

namespace neon {

class ProceduralAudio final : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal musicVolume READ musicVolume WRITE setMusicVolume NOTIFY settingsChanged)
    Q_PROPERTY(qreal effectsVolume READ effectsVolume WRITE setEffectsVolume NOTIFY settingsChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY settingsChanged)
    Q_PROPERTY(int district READ district WRITE setDistrict NOTIFY contextChanged)
    Q_PROPERTY(int contextMode READ contextMode WRITE setContextMode NOTIFY contextChanged)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit ProceduralAudio(QObject *parent = nullptr);
    ~ProceduralAudio() override;
    qreal musicVolume() const { return m_musicVolume; }
    qreal effectsVolume() const { return m_effectsVolume; }
    bool muted() const { return m_muted; }
    int district() const { return m_district; }
    int contextMode() const { return m_contextMode; }
    bool available() const;
    void setMusicVolume(qreal value);
    void setEffectsVolume(qreal value);
    void setMuted(bool value);
    void setDistrict(int value);
    void setContextMode(int value);
    Q_INVOKABLE void playEffect(int effectId);

signals:
    void settingsChanged();
    void contextChanged();

private:
    void apply();
public:
    class Backend;
private:
    std::unique_ptr<Backend> m_backend;
    qreal m_musicVolume = .42;
    qreal m_effectsVolume = .72;
    bool m_muted = false;
    int m_district = 0;
    int m_contextMode = 0;
};

} // namespace neon
