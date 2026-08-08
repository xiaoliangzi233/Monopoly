#pragma once

#include <QObject>
#include <QVariantList>

namespace neon {

class ReleaseNotes final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString releaseDate READ releaseDate CONSTANT)
    Q_PROPERTY(QVariantList sections READ sections CONSTANT)
    Q_PROPERTY(bool unread READ unread NOTIFY unreadChanged)

public:
    explicit ReleaseNotes(QObject *parent = nullptr);

    QString version() const { return m_version; }
    QString title() const { return m_title; }
    QString releaseDate() const { return m_releaseDate; }
    QVariantList sections() const { return m_sections; }
    bool unread() const;

    Q_INVOKABLE void markRead();

signals:
    void unreadChanged();

private:
    QString m_version;
    QString m_title;
    QString m_releaseDate;
    QVariantList m_sections;
};

} // namespace neon
