#include "nodescontroller.h"
#include <QSqlQuery>
#include <QSqlError>
#include <swiftcore.h>

NodesController::NodesController(QObject *parent) : QObject(parent), clickhouse_enabled( SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_ENABLED, false ).toBool() )
{

    if ( clickhouse_enabled ) {
        _ch = QSqlDatabase::addDatabase("QMYSQL", "clickhouse-def");
        _ch.setHostName( SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_HOST, "localhost" ).toString() );
        _ch.setPort(SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_PORT, 9004 ).toUInt() );
        _ch.setUserName(  SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_USER, "default" ).toString()  );
        _ch.setPassword(  SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_PASSWORD, "clickhouse" ).toString()  );
        _ch.setDatabaseName( SwiftBot::appParam( SETTINGS_NAME_CLICKHOUSE_DBNAME, "default" ).toString()  );
        if ( !_ch.open() ) {
            qWarning() << _ch.lastError().text();
        }
    }
}

void NodesController::sendRates() {
    QMap<quint32,double> _r;

    QMap<quint32, double> _maxbids;
    QMap<quint32, double> _minasks;

    const QList<QJsonArray> _allasks( _asks.values() );
    const QList<QJsonArray> _allbids( _bids.values() );
    if ( _allasks.isEmpty() && _allbids.isEmpty() ) {
        return;
    }
    for( auto it1 = _allasks.begin(); it1 != _allasks.end(); it1++ ) {
        const QJsonArray j_a( *it1 );
        for( auto it = j_a.begin(); it != j_a.end(); it++ ) {
            const quint32 pairid = it->toArray().at(0).toString().toUInt();
            const double rate = it->toArray().at(1).toString().toDouble();
            if ( !_minasks.contains( pairid ) ) {
                _minasks.insert( pairid, rate );
            } else {
                if (_minasks.value(pairid) > rate ) {
                    _minasks[pairid] = rate;
                }
            }
        }
    }
    for( auto it1 = _allbids.begin(); it1 != _allbids.end(); it1++ ) {
        const QJsonArray j_a( *it1 );
        for( auto it = j_a.begin(); it != j_a.end(); it++ ) {
            const quint32 pairid = it->toArray().at(0).toString().toUInt();
            const double rate = it->toArray().at(1).toString().toDouble();
            if ( !_maxbids.contains( pairid ) ) {
                _maxbids.insert( pairid, rate );
            } else {
                if (_maxbids.value(pairid) < rate ) {
                    _maxbids[pairid] = rate;
                }
            }
        }
    }

    if ( !_maxbids.isEmpty() && !_minasks.isEmpty() ) {
        for( auto it = _minasks.begin(); it != _minasks.end(); it++ ) {
            const quint32 pair = it.key();
            if ( _maxbids.contains( pair ) ) {
                const double ask = it.value();
                const double bid = _maxbids.value( pair );
                const double rate = bid > ask ?  ( bid - ( (bid - ask) / 2 ) )  :  ( ask - ( (ask - bid )/ 2 ) );
                _r[pair] = rate;
            }
        }
    }
    if ( !_r.isEmpty() ) {
        emit pairsRate( _r );
    }
}

void NodesController::maintanceRates() {
    QSqlQuery q("INSERT INTO rates_history SELECT * FROM rates WHERE ts <= ( CURDATE() - INTERVAL 2 DAY ); DELETE FROM rates WHERE ts <= ( CURDATE() - INTERVAL 2 DAY );");
    if ( !q.exec() ) {
        qWarning() << q.lastError().text();
    }
    QTimer::singleShot( 360000, this, &NodesController::maintanceRates );
}

void NodesController::nodeThreadStarted() {
    const QString targetname = qobject_cast<QThread*>( sender() )->objectName();
    // qWarning() << targetname << "thread started";
}

void NodesController::nodeThreadFinished() {
    const QString targetname = qobject_cast<QThread*>( sender() )->objectName();
    qWarning() << targetname << "thread finished";
    emit clientExited();
}

void NodesController::startAllNodes() {
    /* trestimer = new QTimer(this);
    trestimer->setInterval( 2000 );
    connect( trestimer, &QTimer::timeout, this, &NodesController::secStatCalc );
    trestimer->start();*/
    acttimer = new QTimer(this);
    acttimer->setInterval( 12500 );
    connect( acttimer, &QTimer::timeout, this, &NodesController::findOutdatedPairs );
    acttimer->start();

    pubtimer = new QTimer(this);
    pubtimer->setInterval( SwiftCore::getModuleSettings("orderbooks")->value("publish_interval", 1000).toUInt() );
    connect( pubtimer, &QTimer::timeout, this, &NodesController::sendOrderbooks );
    pubtimer->start();

    settingstimer = new QTimer(this);
    settingstimer->setInterval( 15000 );
    connect( settingstimer, &QTimer::timeout, [=](){
        pubtimer->setInterval( SwiftCore::getModuleSettings("orderbooks")->value("publish_interval", 1000).toUInt() );
        pubtimer->stop();
        pubtimer->start();
    } );
    settingstimer->start();

    ratestimer = new QTimer(this);
    ratestimer->setInterval( 60000 );
    connect( ratestimer, &QTimer::timeout, this, &NodesController::sendRates );
    ratestimer->start();

    emit startNodes();
}

void NodesController::onOrderBooks(const QJsonArray &a, const QJsonArray &b) {
    if ( a.isEmpty() && b.isEmpty() ) {
        return;
    }
    results_per_sec++;
    if ( !a.isEmpty() ) {
        QMap<quint32,QJsonArray> _aindexed;
        for( auto it = a.begin(); it != a.end(); it++ ) {
            const quint32 pid = it->toArray().at(0).toString().toUInt();
            if ( pid > 0 ) {
                _aindexed[ pid ].push_back( it->toArray() );
                _last_pair_update[ pid ] = QDateTime::currentMSecsSinceEpoch();
            }
        }
        for ( auto it = _aindexed.begin(); it != _aindexed.end(); it++ ) {
            _asks[ it.key() ] = it.value();
        }

    }

    if ( !b.isEmpty() ) {
        QMap<quint32,QJsonArray> _bindexed;
        for( auto it = b.begin(); it != b.end(); it++ ) {
            const quint32 pid = it->toArray().at(0).toString().toUInt();
            if ( pid > 0 ) {
                _bindexed[ pid ].push_back( it->toArray() );
                _last_pair_update[ pid ] = QDateTime::currentMSecsSinceEpoch();
            }
        }
        for ( auto it = _bindexed.begin(); it != _bindexed.end(); it++ ) {
            _bids[ it.key() ] = it.value();
        }
    }

}

void NodesController::onNodeError(OrderbooksNode::NodeErrors err) {
    qWarning() << err;
}

void NodesController::findOutdatedPairs() {
    const QHash<quint32, quint64> _countersCopy( _last_pair_update );
    for( auto it = _countersCopy.begin(); it != _countersCopy.end(); it++ ) {

        if ( _last_pair_update.value( it.key(), 0 ) == 0 ) {

        } else {
            const quint64 updatetime = it.value();
            quint64 diff = QDateTime::currentMSecsSinceEpoch() - updatetime;
            if ( diff  >= SwiftCore::getModuleSettings("orderbooks")->value("orderbooks_valid_time", 4500 ).toUInt() ) {
                _pair_outdates[ it.key() ].push_back( diff );
                if ( _pair_max_outdated.value( it.key(), 0 ) < diff ) {
                    _pair_max_outdated[ it.key() ] = diff;
                }
                if ( !_current_outdated_list.contains( it.key() ) ) {
                    _current_outdated_list.push_back( it.key() );
                }
            } else {
                if ( _current_outdated_list.contains( it.key() ) ) {
                    _current_outdated_list.removeOne( it.key() );
                }

            }
        }
    }
    for( auto it = _current_outdated_list.begin(); it != _current_outdated_list.end(); it++ ) {
        _asks.remove( *it );
        _bids.remove( *it );
        _current_outdated_list.removeOne( *it );
    }
}

void NodesController::secStatCalc() {
    _results.push_back( results_per_sec );
    midrate = std::accumulate(_results.begin(), _results.end(), 0 ) / _results.size();
    results_per_sec = 0;
}

void NodesController::sendStats() {
    QJsonObject stat;
    stat["ts"] = QString::number( QDateTime::currentSecsSinceEpoch() );
    QJsonArray _pairs;
    const QList<quint32> p( _asks.keys() );
    for( auto it =p.begin(); it != p.end(); it++ ) {
        _pairs.push_back( QString::number( *it ) );
    }
    stat["pairs"] = _pairs;
    QJsonObject outdated;
    for( auto it = _current_outdated_list.begin(); it != _current_outdated_list.end(); it++ ) {
        const quint64 cdiff =  QDateTime::currentSecsSinceEpoch() - _last_pair_update.value( *it, 0 ) ;
        if ( cdiff > 0 ) {
            outdated[ QString::number( *it) ] = QString::number( cdiff );
        }
    }
    stat["pair_by_secs"] = QString::number( midrate );
    stat["outdated_pairs"] = outdated;
    emit summaryStats( "swiftbot.orderbooks.status", stat );
}

void NodesController::sendOrderbooks() {       
    QElapsedTimer timer;
    timer.start();
    const QList<QJsonArray> _allasks( _asks.values() );
    const QList<QJsonArray> _allbids( _bids.values() );
    if ( _allasks.isEmpty() && _allbids.isEmpty() ) {
        return;
    }
    QJsonArray accresa;
    QJsonArray accresb;

    if ( clickhouse_enabled ) {
        QSqlQuery q( _ch );
        QString _q_asks("INSERT INTO `asks` (d,ts,pair,rate,amount) VALUES ");
        QStringList _asksparts;
        QString _q_bids("INSERT INTO `bids` (d,ts,pair,rate,amount) VALUES ");
        QStringList _bidsparts;

        for( auto it = _allasks.begin(); it != _allasks.end(); it++ ) {
            QJsonArray _i( *it );
            for( auto rit = _i.begin(); rit != _i.end(); rit++ ) {
                _asksparts.push_back("(toDate(now()),now(),"+QString::number( rit->toArray().at(0).toString().toUInt())+","+QString::number( rit->toArray().at(1).toString().toDouble(),'f',8)+","+QString::number( rit->toArray().at(2).toString().toDouble(),'f',8)+")");
            }
            accresa.push_back( *it );
        }
        for( auto it = _allbids.begin(); it != _allbids.end(); it++ ) {
            QJsonArray _i( *it );
            for( auto rit = _i.begin(); rit != _i.end(); rit++ ) {
                _bidsparts.push_back("(toDate(now()),now(),"+QString::number( rit->toArray().at(0).toString().toUInt())+","+QString::number( rit->toArray().at(1).toString().toDouble(),'f',8)+","+QString::number( rit->toArray().at(2).toString().toDouble(),'f',8)+")");
            }
            accresb.push_back( *it );
        }
        _q_asks += _asksparts.join(",");
        _q_bids += _bidsparts.join(",");
        if ( !q.exec( _q_asks ) ) {
            qWarning() << q.lastError().text();
        }
        if ( !q.exec( _q_bids ) ) {
            qWarning() << q.lastError().text();
        }
        if ( timer.elapsed() > 100 ) {
            qWarning() << "The slow operation took" << timer.elapsed() << "milliseconds";
        }
    } else {
        for( auto it = _allasks.begin(); it != _allasks.end(); it++ ) {
            accresa.push_back( *it );
        }
        for( auto it = _allbids.begin(); it != _allbids.end(); it++ ) {
            accresb.push_back( *it );
        }
    }
    QJsonObject j_res;
    j_res["asks"] = accresa;
    j_res["bids"] = accresb;
    emit orderbooksAccumulated( FEED_ORDERBOOKS_SNAPSHOT, j_res );
}
