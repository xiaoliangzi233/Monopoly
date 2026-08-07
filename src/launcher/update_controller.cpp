#include "launcher/update_controller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>

namespace neon::launcher {

UpdateController::UpdateController(QObject *parent) : QObject(parent)
{
    QSettings settings;
    const QString github = settings.value(QStringLiteral("updates/githubManifest"), QStringLiteral(NEON_GITHUB_MANIFEST_URL)).toString();
    const QString gitee = settings.value(QStringLiteral("updates/giteeManifest"), QStringLiteral(NEON_GITEE_MANIFEST_URL)).toString();
    if (!github.isEmpty()) m_sources.append(QUrl(github));
    if (!gitee.isEmpty()) m_sources.append(QUrl(gitee));
}

bool UpdateController::updateAvailable() const
{
    return !m_manifest.version.isNull() && QVersionNumber::compare(m_manifest.version, QVersionNumber::fromString(NEON_VERSION)) > 0;
}

void UpdateController::configureSources(const QString &githubUrl, const QString &giteeUrl)
{
    QSettings settings;
    settings.setValue(QStringLiteral("updates/githubManifest"), githubUrl);
    settings.setValue(QStringLiteral("updates/giteeManifest"), giteeUrl);
    m_sources.clear();
    if (QUrl(githubUrl).isValid()) m_sources.append(QUrl(githubUrl));
    if (QUrl(giteeUrl).isValid()) m_sources.append(QUrl(giteeUrl));
    setStatus(QStringLiteral("更新源已保存"));
}

void UpdateController::checkForUpdates(const QString &channel)
{
    if (m_busy) return;
    m_channel = channel == QStringLiteral("beta") ? channel : QStringLiteral("stable");
    if (m_sources.isEmpty()) {
        setStatus(QStringLiteral("发布仓库尚未配置，当前可直接启动游戏"));
        return;
    }
    if (!m_verifier.isConfigured()) {
        setStatus(QStringLiteral("发布公钥尚未配置，安全策略已阻止检查更新"));
        return;
    }
    m_manifest = {};
    m_candidates.clear();
    m_pendingManifests = m_sources.size();
    setBusy(true);
    setStatus(QStringLiteral("正在检查 GitHub 与 Gitee…"));
    for (auto source : std::as_const(m_sources)) {
        QString path = source.path();
        if (path.endsWith(QStringLiteral("/stable.json")) || path.endsWith(QStringLiteral("/beta.json"))) {
            path = path.left(path.lastIndexOf(QLatin1Char('/')) + 1) + m_channel + QStringLiteral(".json");
            source.setPath(path);
        }
        fetchManifest(source);
    }
}

void UpdateController::fetchManifest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    request.setRawHeader("User-Agent", "NeonTycoonLauncher/" NEON_VERSION);
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleManifestReply(reply); });
}

void UpdateController::handleManifestReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        const auto document = QJsonDocument::fromJson(reply->readAll());
        QString error;
        const auto candidate = UpdateManifest::fromJson(document.object(), &error);
        const auto launcherVersion = QVersionNumber::fromString(QStringLiteral(NEON_VERSION));
        if (error.isEmpty() && candidate.channel == m_channel && m_verifier.verify(candidate, &error)
            && QVersionNumber::compare(launcherVersion, candidate.minimumLauncherVersion) >= 0)
            m_candidates.append(candidate);
    }
    reply->deleteLater();
    if (--m_pendingManifests > 0) return;
    std::sort(m_candidates.begin(), m_candidates.end(), [](const auto &left, const auto &right) {
        return QVersionNumber::compare(left.version, right.version) > 0;
    });
    if (!m_candidates.isEmpty()) m_manifest = m_candidates.first();
    setBusy(false);
    if (m_manifest.version.isNull()) setStatus(QStringLiteral("未获得可信更新清单，保留当前版本"));
    else if (updateAvailable()) setStatus(QStringLiteral("发现新版本 %1").arg(m_manifest.version.toString()));
    else setStatus(QStringLiteral("当前已是最新版本"));
    emit updateChanged();
}

void UpdateController::downloadUpdate()
{
    if (m_busy || !updateAvailable()) return;
    m_downloadCandidates.clear();
    for (const auto &candidate : std::as_const(m_candidates)) {
        if (candidate.version == m_manifest.version && candidate.size == m_manifest.size
            && candidate.sha256 == m_manifest.sha256)
            m_downloadCandidates.append(candidate);
    }
    if (m_downloadCandidates.isEmpty()) m_downloadCandidates.append(m_manifest);
    m_downloadCandidate = 0;
    m_verifiedPackage.clear();
    setBusy(true);
    startCandidateDownload();
}

void UpdateController::startCandidateDownload()
{
    if (m_downloadCandidate >= m_downloadCandidates.size()) {
        setStatus(QStringLiteral("所有更新源均下载失败，已保留当前版本"));
        setBusy(false);
        emit updateChanged();
        return;
    }
    const auto &candidate = m_downloadCandidates.at(m_downloadCandidate);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/updates/staging");
    QDir().mkpath(directory);
    const QString path = directory + QStringLiteral("/NeonTycoon-%1.exe.part").arg(candidate.version.toString());
    if (m_download.isOpen()) m_download.close();
    m_download.setFileName(path);
    if (!m_download.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setStatus(QStringLiteral("无法创建更新暂存文件：%1").arg(m_download.errorString()));
        setBusy(false);
        return;
    }
    m_hash.reset();
    m_downloadFailed = false;
    m_progress = 0;
    emit progressChanged();
    setStatus(m_downloadCandidate == 0 ? QStringLiteral("正在下载并验证更新…")
                                       : QStringLiteral("主更新源失败，正在切换备用源…"));
    QNetworkRequest request(candidate.downloadUrl);
    request.setTransferTimeout(30000);
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        const QByteArray chunk = reply->readAll();
        if (m_download.write(chunk) != chunk.size()) m_downloadFailed = true;
        m_hash.addData(chunk);
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_progress = total > 0 ? qreal(received) / qreal(total) : 0;
        emit progressChanged();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleDownloadReply(reply); });
}

void UpdateController::handleDownloadReply(QNetworkReply *reply)
{
    const auto candidate = m_downloadCandidates.at(m_downloadCandidate);
    const QByteArray tail = reply->readAll();
    if (m_download.write(tail) != tail.size()) m_downloadFailed = true;
    m_hash.addData(tail);
    m_download.flush();
    m_download.close();
    const QFileInfo info(m_download.fileName());
    const bool valid = !m_downloadFailed && reply->error() == QNetworkReply::NoError
        && info.size() == candidate.size && m_hash.result() == candidate.sha256;
    reply->deleteLater();
    if (!valid) {
        QFile::remove(m_download.fileName());
        ++m_downloadCandidate;
        startCandidateDownload();
        return;
    }
    m_verifiedPackage = m_download.fileName();
    m_verifiedPackage.chop(5);
    QFile::remove(m_verifiedPackage);
    if (!QFile::rename(m_download.fileName(), m_verifiedPackage)) {
        QFile::remove(m_download.fileName());
        m_verifiedPackage.clear();
        setStatus(QStringLiteral("无法完成更新包原子落盘，已保留当前版本"));
    } else {
        setStatus(QStringLiteral("更新已验证，可以自动安装"));
    }
    setBusy(false);
    emit updateChanged();
}

void UpdateController::installVerifiedUpdate()
{
    if (m_verifiedPackage.isEmpty() || !QFileInfo::exists(m_verifiedPackage)) return;
    if (QProcess::startDetached(m_verifiedPackage, {QStringLiteral("/S")})) QCoreApplication::quit();
    else setStatus(QStringLiteral("无法启动已验证安装包"));
}

void UpdateController::launchGame()
{
    const QString game = QCoreApplication::applicationDirPath() + QStringLiteral("/NeonTycoon.exe");
    if (!QFileInfo::exists(game)) { setStatus(QStringLiteral("未找到游戏程序，请重新安装")); return; }
    if (QProcess::startDetached(game)) QCoreApplication::quit();
    else setStatus(QStringLiteral("游戏启动失败"));
}

void UpdateController::setStatus(const QString &status)
{
    if (m_status == status) return;
    m_status = status;
    emit statusChanged();
}

void UpdateController::setBusy(bool busy)
{
    if (m_busy == busy) return;
    m_busy = busy;
    emit busyChanged();
}

} // namespace neon::launcher
