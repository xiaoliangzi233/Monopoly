#pragma once

#include "launcher/update_manifest.h"

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>

class QNetworkReply;

namespace neon::launcher {

class UpdateController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
    Q_PROPERTY(bool packageReady READ packageReady NOTIFY updateChanged)

public:
    explicit UpdateController(QObject *parent = nullptr);
    QString status() const { return m_status; }
    QString latestVersion() const { return m_manifest.version.toString(); }
    qreal progress() const { return m_progress; }
    bool busy() const { return m_busy; }
    bool updateAvailable() const;
    bool packageReady() const { return !m_verifiedPackage.isEmpty(); }

    Q_INVOKABLE void checkForUpdates(const QString &channel = QStringLiteral("stable"));
    Q_INVOKABLE void downloadUpdate();
    Q_INVOKABLE void installVerifiedUpdate();
    Q_INVOKABLE void launchGame();
    Q_INVOKABLE void configureSources(const QString &githubUrl, const QString &giteeUrl);

signals:
    void statusChanged();
    void progressChanged();
    void busyChanged();
    void updateChanged();

private:
    void fetchManifest(const QUrl &url);
    void handleManifestReply(QNetworkReply *reply);
    void startCandidateDownload();
    void handleDownloadReply(QNetworkReply *reply);
    void setStatus(const QString &status);
    void setBusy(bool busy);

    QNetworkAccessManager m_network;
    SignatureVerifier m_verifier{QByteArray(NEON_UPDATE_PUBLIC_KEY_HEX)};
    QList<QUrl> m_sources;
    QList<UpdateManifest> m_candidates;
    QList<UpdateManifest> m_downloadCandidates;
    UpdateManifest m_manifest;
    QString m_channel = QStringLiteral("stable");
    QString m_status = QStringLiteral("准备就绪");
    qreal m_progress = 0.0;
    bool m_busy = false;
    int m_pendingManifests = 0;
    int m_downloadCandidate = 0;
    bool m_downloadFailed = false;
    QFile m_download;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    QString m_verifiedPackage;
};

} // namespace neon::launcher
