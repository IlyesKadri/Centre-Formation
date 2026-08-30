#include <QApplication>
#include <iostream>
#include "connection.h"
#include "birdconfig.h"
#include "birdemailservice.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "TEST BIRD SERVICE AVEC BIRDCONFIG.H" << std::endl;
    std::cout << "Endpoint     : " << BirdEmailService::getApiEndpoint().toStdString() << std::endl;
    std::cout << "Sender Name  : " << BirdEmailService::getSenderName().toStdString() << std::endl;
    std::cout << "Sender Mail  : " << BirdEmailService::getSenderEmail().toStdString() << std::endl;
    std::cout << "Config API_KEY default test : " << BirdConfig::API_KEY << std::endl;
    std::cout << "========================================" << std::endl;

    BirdEmailService service;
    QObject::connect(&service, &BirdEmailService::emailEnvoyeSucces, [](int idInsc, const QString &dest, const QString &msgId, const QString &status) {
        std::cout << "\n=== SUCCES BIRD EMAIL ===" << std::endl;
        std::cout << "Destinataire : " << dest.toStdString() << " | MsgId : " << msgId.toStdString() << " | Statut : " << status.toStdString() << std::endl;
        QApplication::quit();
    });

    QObject::connect(&service, &BirdEmailService::emailEchec, [](int idInsc, const QString &dest, const QString &err) {
        std::cout << "\n=== RESULTAT OBTENU ===" << std::endl;
        std::cout << "Destinataire : " << dest.toStdString() << std::endl;
        std::cout << "Message      : " << err.toStdString() << std::endl;
        QApplication::quit();
    });

    std::cout << "\nLancement test Sandbox Bird (Hello World -> delivered@messagebird.dev)..." << std::endl;
    service.testBirdEmail();

    return a.exec();
}
