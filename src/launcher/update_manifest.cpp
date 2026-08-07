#include "launcher/update_manifest.h"

#ifdef NEON_HAVE_SODIUM
#include <sodium.h>
#endif

#include <algorithm>

namespace neon::launcher {

QByteArray UpdateManifest::signedMessage() const
{
    return channel.toUtf8() + '\n' + version.toString().toUtf8() + '\n'
        + minimumLauncherVersion.toString().toUtf8() + '\n' + downloadUrl.toEncoded() + '\n'
        + QByteArray::number(size) + '\n' + sha256.toHex();
}

bool UpdateManifest::isValid(QString *error) const
{
    const bool channelValid = channel == QStringLiteral("stable") || channel == QStringLiteral("beta");
    const bool urlValid = downloadUrl.isValid() && downloadUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
    const bool valid = channelValid && !version.isNull() && !minimumLauncherVersion.isNull() && urlValid
        && size > 0 && size <= qint64(2) * 1024 * 1024 * 1024 && sha256.size() == 32 && signature.size() == 64;
    if (!valid && error) *error = QStringLiteral("更新清单字段无效或不安全");
    else if (error) error->clear();
    return valid;
}

UpdateManifest UpdateManifest::fromJson(const QJsonObject &object, QString *error)
{
    UpdateManifest manifest;
    manifest.channel = object.value(QStringLiteral("channel")).toString();
    manifest.version = QVersionNumber::fromString(object.value(QStringLiteral("version")).toString());
    manifest.minimumLauncherVersion = QVersionNumber::fromString(object.value(QStringLiteral("minimumLauncherVersion")).toString());
    manifest.downloadUrl = QUrl(object.value(QStringLiteral("url")).toString());
    manifest.size = qint64(object.value(QStringLiteral("size")).toDouble());
    manifest.sha256 = QByteArray::fromHex(object.value(QStringLiteral("sha256")).toString().toLatin1());
    manifest.signature = QByteArray::fromBase64(object.value(QStringLiteral("signature")).toString().toLatin1());
    const bool valid = manifest.isValid(error);
    Q_UNUSED(valid)
    return manifest;
}

SignatureVerifier::SignatureVerifier(const QByteArray &publicKeyHex) : m_publicKey(QByteArray::fromHex(publicKeyHex))
{
#ifdef NEON_HAVE_SODIUM
    const int initialized = sodium_init();
    Q_UNUSED(initialized)
#endif
}

bool SignatureVerifier::isConfigured() const
{
#ifdef NEON_HAVE_SODIUM
    if (m_publicKey.size() != crypto_sign_PUBLICKEYBYTES) return false;
    return std::any_of(m_publicKey.cbegin(), m_publicKey.cend(), [](char value) { return value != 0; });
#else
    return false;
#endif
}

bool SignatureVerifier::verify(const UpdateManifest &manifest, QString *error) const
{
    if (!isConfigured()) {
        if (error) *error = QStringLiteral("启动器尚未配置发布公钥，已拒绝远程更新");
        return false;
    }
#ifdef NEON_HAVE_SODIUM
    const QByteArray message = manifest.signedMessage();
    const int result = crypto_sign_verify_detached(reinterpret_cast<const unsigned char *>(manifest.signature.constData()),
        reinterpret_cast<const unsigned char *>(message.constData()), size_t(message.size()),
        reinterpret_cast<const unsigned char *>(m_publicKey.constData()));
    if (result != 0) {
        if (error) *error = QStringLiteral("更新清单数字签名无效");
        return false;
    }
    if (error) error->clear();
    return true;
#else
    Q_UNUSED(manifest)
    if (error) *error = QStringLiteral("当前构建缺少 Ed25519 校验后端");
    return false;
#endif
}

} // namespace neon::launcher
