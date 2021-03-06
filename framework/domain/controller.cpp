/*
    Copyright (c) 2016 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/
#include "controller.h"

#include <QQmlEngine>
#include <QMetaProperty>
#include <sink/log.h>

using namespace Kube;

ControllerAction::ControllerAction()
    : QObject()
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

void ControllerAction::execute()
{
    emit triggered();
}

void Controller::clear()
{
    auto meta = metaObject();
    for (auto i = meta->propertyOffset(); i < meta->propertyCount(); i++) {
        auto property = meta->property(i);
        setProperty(property.name(), QVariant());
    }
    for (const auto &p : dynamicPropertyNames()) {
        setProperty(p, QVariant());
    }
}

void Controller::run(const KAsync::Job<void> &job)
{
    auto jobToExec = job;
    jobToExec.onError([] (const KAsync::Error &error) {
        SinkWarningCtx(Sink::Log::Context{"controller"}) << "Error while executing job: " << error.errorMessage;
    });
    //TODO handle error
    //TODO attach a log context to the execution that we can gather from the job?
    jobToExec.exec();
}
