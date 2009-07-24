/*
 * Copyright 2009 Zsombor Gegesy <gzsombor@gmail.com>
 * Copyright 2009 Benjamin Meyer <ben@meyerhome.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef FILTERSUBSCRIPTION_H
#define FILTERSUBSCRIPTION_H

#include <qdatetime.h>
#include <qurl.h>
#include <qmetatype.h>

class FilterSubscription
{

public:
    FilterSubscription();

    int priority() const;
    void setPriority(int priority);

    void setName(const QString &name);
    QString name() const;

    void setUrl(const QUrl &url);
    QUrl url() const;

    void setLastFetchedDate(const QDate &date);
    QDate lastFetchedDate() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    static void load(QDataStream &in, FilterSubscription &metaData);
    static void save(QDataStream &out, const FilterSubscription &metaData);

    // static void parseUrl(const QUrl &url);
    // "abp:subscribe?location=http%3A%2F%2Feasylist.adblockplus.org%2Feasylist.txt&title=EasyList"

private:
    int m_priority;
    QString m_name;
    QByteArray m_url;
    QDate m_lastFetchedDate;
    bool m_enabled;
};

QDataStream &operator<<(QDataStream &, const FilterSubscription &);
QDataStream &operator>>(QDataStream &, FilterSubscription &);

Q_DECLARE_METATYPE(FilterSubscription)
Q_DECLARE_METATYPE(QList<FilterSubscription>)

#endif // FILTERSUBSCRIPTION_H
