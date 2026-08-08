#include "app/release_notes.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace neon {

ReleaseNotes::ReleaseNotes(QObject *parent) : QObject(parent)
{
    QFile file(QStringLiteral(":/qt/qml/NeonTycoon/resources/release_notes.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        file.setFileName(QStringLiteral(":/NeonTycoon/resources/release_notes.json"));
        if (!file.open(QIODevice::ReadOnly)) return;
    }
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    m_version = root.value(QStringLiteral("version")).toString();
    m_title = root.value(QStringLiteral("title")).toString();
    m_releaseDate = root.value(QStringLiteral("releaseDate")).toString();
    for (const auto &sectionValue : root.value(QStringLiteral("sections")).toArray()) {
        const auto section = sectionValue.toObject();
        QStringList items;
        for (const auto &item : section.value(QStringLiteral("items")).toArray()) items.append(item.toString());
        m_sections.append(QVariantMap{{QStringLiteral("title"), section.value(QStringLiteral("title")).toString()},
                                     {QStringLiteral("items"), items}});
    }
}

bool ReleaseNotes::unread() const
{
    return !m_version.isEmpty()
        && QSettings().value(QStringLiteral("ui/lastReadReleaseNotesVersion")).toString() != m_version;
}

void ReleaseNotes::markRead()
{
    if (!unread()) return;
    QSettings().setValue(QStringLiteral("ui/lastReadReleaseNotesVersion"), m_version);
    emit unreadChanged();
}

} // namespace neon
