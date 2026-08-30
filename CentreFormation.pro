QT       += core gui sql printsupport charts network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    birdemailservice.cpp \
    certificateverificationservice.cpp \
    connection.cpp \
    cours.cpp \
    courswidget.cpp \
    dashboard.cpp \
    groqrecommendationservice.cpp \
    inscription.cpp \
    inscriptionswidget.cpp \
    main.cpp \
    mainwindow.cpp \
    qrcode.cpp \
    recommandationwidget.cpp \
    stagiaire.cpp \
    stagiaireswidget.cpp \
    statistiques.cpp \
    statistiqueswidget.cpp

HEADERS += \
    birdconfig.h \
    birdemailservice.h \
    certificateverificationservice.h \
    connection.h \
    cours.h \
    courswidget.h \
    dashboard.h \
    groqrecommendationservice.h \
    inscription.h \
    inscriptionswidget.h \
    mainwindow.h \
    qrcode.h \
    recommandationwidget.h \
    stagiaire.h \
    stagiaireswidget.h \
    statistiques.h \
    statistiqueswidget.h

FORMS += \
    cours.ui \
    inscriptions.ui \
    mainwindow.ui \
    recommandations.ui \
    stagiaires.ui \
    statistiques.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
