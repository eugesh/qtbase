// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "helpers.h"
#include <qdebug.h>

using namespace Qt::StringLiterals;

QString typeToName(const QString &name)
{
    QString ret = name;
    return ret.replace(QStringLiteral("::"), QStringLiteral("_"));
}

QString includeGuard(const QString &filename)
{
    QString guard = filename.toUpper();

    for (int i = 0; i < guard.size(); ++i) {
        if (!guard.at(i).isLetterOrNumber())
            guard[i] = u'_';
    }

    return guard;
}

template<typename T>
static QString joinArguments(const QList<Tracepoint::Argument> &args, T joinFunction)
{
    QString ret;
    bool first = true;

    for (const Tracepoint::Argument &arg : args) {
        if (!first)
            ret += ", "_L1;

        ret += joinFunction(arg);

        first = false;
    }

    return ret;
}

QString formatFunctionSignature(const QList<Tracepoint::Argument> &args)
{
    return joinArguments(args, [](const Tracepoint::Argument &arg) {
            return QStringLiteral("%1 %2").arg(arg.type).arg(arg.name);
    });
}

QString formatParameterList(const Provider &provider, const QList<Tracepoint::Argument> &args, const QList<Tracepoint::Field> &fields, ParamType type)
{
    if (type == LTTNG) {
        QString ret;

        for (int i = 0; i < args.size(); i++) {
            const Tracepoint::Argument &arg = args[i];
            const Tracepoint::Field &field = fields[i];
            if (field.backendType.backendType == Tracepoint::Field::FlagType)
                ret += ", trace_convert_"_L1 + typeToName(arg.type) + "("_L1 + arg.name + ")"_L1;
            else
                ret += ", "_L1 + arg.name;
        }
        return ret;
    }

    auto findEnumeration = [](const QList<TraceEnum> &enums, const QString &name) {
        for (const auto &e : enums) {
            if (e.name == name)
                return e;
        }
        return TraceEnum();
    };

    if (type == CTF) {
        QString ret;

        for (int i = 0; i < args.size(); i++) {
            const Tracepoint::Argument &arg = args[i];
            const Tracepoint::Field &field = fields[i];
            if (arg.arrayLen > 1) {
                ret += ", trace::toByteArrayFromArray("_L1 + arg.name + ", "_L1 + QString::number(arg.arrayLen) + ") "_L1;
            } else if (field.backendType.backendType == Tracepoint::Field::EnumeratedType) {
                const TraceEnum &e = findEnumeration(provider.enumerations, arg.type);
                QString integerType;
                if (e.valueSize == 8)
                    integerType = QStringLiteral("quint8");
                else if (e.valueSize == 16)
                    integerType = QStringLiteral("quint16");
                else
                    integerType = QStringLiteral("quint32");
                ret += ", trace::toByteArrayFromEnum<"_L1 + integerType + ">("_L1 + arg.name + ")"_L1;
            } else if (field.backendType.backendType == Tracepoint::Field::FlagType) {
                ret += ", trace::toByteArrayFromFlags("_L1 + arg.name + ")"_L1;
            } else if (field.backendType.backendType == Tracepoint::Field::String) {
                ret += ", trace::toByteArrayFromCString("_L1 + arg.name + ")"_L1;
            } else {
                ret += ", "_L1 + arg.name;
            }
        }
        return ret;
    }

    return joinArguments(args, [](const Tracepoint::Argument &arg) { return arg.name; });
}
