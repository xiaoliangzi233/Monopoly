#pragma once

#include <QJsonObject>
#include <QUrl>
#include <QVersionNumber>

namespace neon::launcher {

struct UpdateManifest {
    QString channel;
    QVersionNumber version;
    QVersionNumber minimumLauncherVersion;
    QUrl downloadUrl;
    qint64 size = 0;
    QByteArray sha256;
    QByteArray signature;

    [[nodiscard]] QByteArray signedMessage() const;
    [[nodiscard]] bool isValid(QString *error = nullptr) const;
    static UpdateManifest fromJson(const QJsonObject &object, QString *error = nullptr);
};

class SignatureVerifier final {
public:
    explicit SignatureVerifier(const QByteArray &publicKeyHex);
    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] bool verify(const UpdateManifest &manifest, QString *error = nullptr) const;

private:
    QByteArray m_publicKey;
};

} // namespace neon::launcher
