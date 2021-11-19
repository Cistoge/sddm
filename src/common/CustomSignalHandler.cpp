/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "CustomSignalHandler.h"
#include <QDebug>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

int CustomSignalHandler::signalFd[2];

CustomSignalHandler::CustomSignalHandler()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd)) {
        qWarning() << "Couldn't create a socketpair";
        return;
    }

    m_handler = new QSocketNotifier(signalFd[1], QSocketNotifier::Read, this);
    connect(m_handler, &QSocketNotifier::activated, this, &CustomSignalHandler::handleSignal);
}

CustomSignalHandler::~CustomSignalHandler()
{
    for (int sig : qAsConst(m_signalsRegistered)) {
        signal(sig, nullptr);
    }
    close(signalFd[0]);
    close(signalFd[1]);
}

void CustomSignalHandler::addSignal(int signalToTrack)
{
    m_signalsRegistered.insert(signalToTrack);
    signal(signalToTrack, signalHandler);
}

void CustomSignalHandler::signalHandler(int signal)
{
    ::write(signalFd[0], &signal, sizeof(signal));
}

void CustomSignalHandler::handleSignal()
{
    m_handler->setEnabled(false);
    int signal;
    ::read(signalFd[1], &signal, sizeof(signal));
    m_handler->setEnabled(true);

    Q_EMIT signalReceived(signal);
}

CustomSignalHandler *CustomSignalHandler::self()
{
    static CustomSignalHandler s_self;
    return &s_self;
}
