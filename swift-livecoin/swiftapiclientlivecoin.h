#ifndef SWIFTAPICLIENTLIVECOIN_H
#define SWIFTAPICLIENTLIVECOIN_H

#include <QObject>
#include "../swift-corelib/swiftapiclient.h"
#include "../swift-corelib/swiftcore.h"
#include <QMutex>
class SwiftApiClientLivecoin : public SwiftApiClient
{
    Q_OBJECT
public:
    SwiftApiClientLivecoin(QObject *parent = nullptr);

signals:


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

};

#endif // SWIFTAPICLIENTLIVECOIN_H
