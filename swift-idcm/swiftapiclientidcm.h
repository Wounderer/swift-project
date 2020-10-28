#ifndef SWIFTAPICLIENTIDCM_H
#define SWIFTAPICLIENTIDCM_H

#include <QObject>
#include <swiftapiclient.h>
#include <swiftcore.h>

class SwiftApiClientIDCM : public SwiftApiClient
{
    Q_OBJECT
public:
    SwiftApiClientIDCM(QObject* parent = nullptr);

    // SwiftApiClient interface
public:

    QString getExchangeName() const override;
    QString getApiVersionString() override;
    void getCurrencies(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void getMarkets(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void getOrderbooks(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void orderPlace(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void orderCancel(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void orderGet(const QJsonObject &j_params, const quint64 &async_uuid) override;

    void withdrawGetLimits(const QJsonObject &j_params, const quint64 &async_uuid) override;

    void withdrawHistory(const QJsonObject &j_params, const quint64 &async_uuid) override;

    void withdrawCreate(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void withdrawInner(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void withdrawGetFee(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void getBalances(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void getDeposits(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void getDepositAddress(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void tradeHistory(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void tradeOpenOrders(const QJsonObject &j_params, const quint64 &async_uuid) override;
    void tradeGetFee(const QJsonObject &j_params, const quint64 &async_uuid) override;

private:
    QString rest_url = "https://api.IDCM.cc:8323/api/v1/";

    void getHistoryOrders(const QJsonObject &j_params, const quint64 &async_uuid, const QJsonArray& orderType, const QString& methodName);
    void buildPostRequest_withSignature(const QString& api_path, const QJsonObject &params, const quint64 &uuid);
};

#endif // SWIFTAPICLIENTIDCM_H
