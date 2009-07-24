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

#include "filtersubscription.h"

FilterSubscription::FilterSubscription()
    : m_priority(0)
    , m_enabled(false)
{
}

void FilterSubscription::setPriority(int priority)
{
    m_priority = priority;
}

int FilterSubscription::priority() const
{
    return m_priority;
}

void FilterSubscription::setName(const QString &name)
{
    m_name = name;
}

QString FilterSubscription::name() const
{
    return m_name;
}

void FilterSubscription::setUrl(const QUrl &url)
{
    m_url = url.toEncoded();
}

QUrl FilterSubscription::url() const
{
    return QUrl::fromEncoded(m_url);
}

void FilterSubscription::setLastFetchedDate(const QDate &date)
{
    m_lastFetchedDate = date;
}

QDate FilterSubscription::lastFetchedDate() const
{
    return m_lastFetchedDate;
}

void FilterSubscription::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool FilterSubscription::isEnabled() const
{
    return m_enabled;
}

QDataStream &operator>>(QDataStream &in, FilterSubscription &subscription)
{
    FilterSubscription::load(in, subscription);
    return in;
}

QDataStream &operator<<(QDataStream &out, const FilterSubscription &subscription)
{
    FilterSubscription::save(out, subscription);
    return out;
}

void FilterSubscription::load(QDataStream &in, FilterSubscription &subscription)
{
    in >> subscription.m_priority;
    in >> subscription.m_name;
    in >> subscription.m_url;
    in >> subscription.m_lastFetchedDate;
    in >> subscription.m_enabled;
}

void FilterSubscription::save(QDataStream &out, const FilterSubscription &subscription)
{
    out << subscription.priority();
    out << subscription.name();
    out << subscription.m_url;
    out << subscription.lastFetchedDate();
    out << subscription.isEnabled();
}
