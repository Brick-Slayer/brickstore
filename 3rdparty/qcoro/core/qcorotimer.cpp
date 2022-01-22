// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorotimer.h"

#include <QMetaObject>
#include <QPointer>

using namespace QCoro::detail;

QCoroTimer::WaitForTimeoutOperation::WaitForTimeoutOperation(QTimer *timer)
    : mTimer(timer) {}

QCoroTimer::WaitForTimeoutOperation::WaitForTimeoutOperation(QTimer &timer)
    : WaitForTimeoutOperation(&timer) {}

bool QCoroTimer::WaitForTimeoutOperation::await_ready() const noexcept {
    return !mTimer || !mTimer->isActive();
}

void QCoroTimer::WaitForTimeoutOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) {
    if (mTimer && mTimer->isActive()) {
        mConn = QObject::connect(mTimer, &QTimer::timeout, [this, awaitingCoroutine]() mutable {
            QObject::disconnect(mConn);
            awaitingCoroutine.resume();
        });
    } else {
        awaitingCoroutine.resume();
    }
}

void QCoroTimer::WaitForTimeoutOperation::await_resume() const {}

QCoroTimer::QCoroTimer(QTimer *timer)
    : mTimer(timer) {}

QCoroTimer::WaitForTimeoutOperation QCoroTimer::waitForTimeout() const {
    return WaitForTimeoutOperation{*mTimer};
}

