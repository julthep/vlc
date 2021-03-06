/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef MLBASEMODEL_HPP
#define MLBASEMODEL_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vlc_common.h"

#include <memory>
#include <QObject>
#include <QAbstractListModel>
#include "vlc_media_library.h"
#include "mlqmltypes.hpp"
#include "medialib.hpp"
#include <memory>
#include "mlevent.hpp"
#include "mlqueryparams.hpp"
#include "util/listcache.hpp"

class MediaLib;

class MLBaseModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MLBaseModel(QObject *parent = nullptr);
    virtual ~MLBaseModel();

    Q_INVOKABLE void sortByColumn(QByteArray name, Qt::SortOrder order);

    Q_PROPERTY( MLParentId parentId READ parentId WRITE setParentId NOTIFY parentIdChanged RESET unsetParentId )
    Q_PROPERTY( MediaLib* ml READ ml WRITE setMl )
    Q_PROPERTY( QString searchPattern READ searchPattern WRITE setSearchPattern )

    Q_PROPERTY( Qt::SortOrder sortOrder READ getSortOrder WRITE setSortOder NOTIFY sortOrderChanged )
    Q_PROPERTY( QString sortCriteria READ getSortCriteria WRITE setSortCriteria NOTIFY sortCriteriaChanged RESET unsetSortCriteria )
    Q_PROPERTY( unsigned int count READ getCount NOTIFY countChanged )

    Q_INVOKABLE virtual QVariant getIdForIndex( QVariant index) const = 0;
    Q_INVOKABLE virtual QVariantList getIdsForIndexes( QVariantList indexes ) const = 0;
    Q_INVOKABLE virtual QVariantList getIdsForIndexes( QModelIndexList indexes ) const = 0;

    Q_INVOKABLE QMap<QString, QVariant> getDataAt(int index);

signals:
    void parentIdChanged();
    void resetRequested();
    void sortOrderChanged();
    void sortCriteriaChanged();
    void countChanged(unsigned int) const;

protected slots:
    void onResetRequested();
    void onLocalSizeAboutToBeChanged(size_t size);
    void onLocalSizeChanged(size_t size);
    void onLocalDataChanged(size_t index, size_t count);

private:
    static void onVlcMlEvent( void* data, const vlc_ml_event_t* event );

protected:
    virtual void clear() = 0;
    virtual vlc_ml_sorting_criteria_t roleToCriteria(int role) const = 0;
    static QString getFirstSymbol(QString str);
    virtual vlc_ml_sorting_criteria_t nameToCriteria(QByteArray) const {
        return VLC_ML_SORTING_DEFAULT;
    }
    virtual QByteArray criteriaToName(vlc_ml_sorting_criteria_t ) const
    {
        return "";
    }

public:
    MLParentId parentId() const;
    void setParentId(MLParentId parentId);
    void unsetParentId();

    MediaLib* ml() const;
    void setMl(MediaLib* ml);

    const QString& searchPattern() const;
    void setSearchPattern( const QString& pattern );

    Qt::SortOrder getSortOrder() const;
    void setSortOder(Qt::SortOrder order);
    const QString getSortCriteria() const;
    void setSortCriteria(const QString& criteria);
    void unsetSortCriteria();

    virtual unsigned int getCount() const = 0;

protected:
    virtual void onVlcMlEvent( const MLEvent &event );

    MLParentId m_parent;

    vlc_medialibrary_t* m_ml;
    MediaLib* m_mediaLib;
    QString m_search_pattern;
    vlc_ml_sorting_criteria_t m_sort;
    bool m_sort_desc;

    std::unique_ptr<vlc_ml_event_callback_t,
                    std::function<void(vlc_ml_event_callback_t*)>> m_ml_event_handle;
    bool m_need_reset;
};

/**
 * Implements a basic sliding window.
 * const_cast & immutable are unavoidable, since all access member functions
 * are marked as const. fetchMore & canFetchMore don't allow for the full size
 * to be known (so the scrollbar would grow as we scroll, until we displayed all
 * elements), and implies having all elements loaded in RAM at all time.
 */
template <typename T>
class MLSlidingWindowModel : public MLBaseModel
{
public:
    static constexpr ssize_t COUNT_UNINITIALIZED =
        ListCache<std::unique_ptr<T>>::COUNT_UNINITIALIZED;

    MLSlidingWindowModel(QObject* parent = nullptr)
        : MLBaseModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (parent.isValid())
            return 0;

        validateCache();

        return m_cache->count();
    }

    void clear() override
    {
        invalidateCache();
        emit countChanged( static_cast<unsigned int>(0) );
    }


    virtual QVariant getIdForIndex( QVariant index ) const override
    {
        T* obj = nullptr;
        if (index.canConvert<int>())
            obj = item( index.toInt() );
        else if ( index.canConvert<QModelIndex>() )
            obj = item( index.value<QModelIndex>().row() );

        if (!obj)
            return {};

        return QVariant::fromValue(obj->getId());
    }

    virtual QVariantList getIdsForIndexes( QModelIndexList indexes ) const override
    {
        QVariantList idList;
        idList.reserve(indexes.length());
        std::transform( indexes.begin(), indexes.end(),std::back_inserter(idList), [this](const QModelIndex& index) -> QVariant {
            T* obj = item( index.row() );
            if (!obj)
                return {};
            return QVariant::fromValue(obj->getId());
        });
        return idList;
    }

    virtual QVariantList getIdsForIndexes( QVariantList indexes ) const override
    {
        QVariantList idList;

        idList.reserve(indexes.length());
        std::transform( indexes.begin(), indexes.end(),std::back_inserter(idList), [this](const QVariant& index) -> QVariant {
            T* obj = nullptr;
            if (index.canConvert<int>())
                obj = item( index.toInt() );
            else if ( index.canConvert<QModelIndex>() )
                obj = item( index.value<QModelIndex>().row() );

            if (!obj)
                return {};

            return QVariant::fromValue(obj->getId());
        });
        return idList;
    }

    unsigned int getCount() const override {
        if (!m_cache || m_cache->count() == COUNT_UNINITIALIZED)
            return 0;
        return static_cast<unsigned int>(m_cache->count());
    }

protected:
    virtual ListCacheLoader<std::unique_ptr<T>> *createLoader() const = 0;

    void validateCache() const
    {
        if (m_cache)
            return;

        auto &threadPool = m_mediaLib->threadPool();
        auto loader = createLoader();
        m_cache.reset(new ListCache<std::unique_ptr<T>>(threadPool, loader));
        connect(&*m_cache, &BaseListCache::localSizeAboutToBeChanged,
                this, &MLSlidingWindowModel<T>::onLocalSizeAboutToBeChanged);
        connect(&*m_cache, &BaseListCache::localSizeChanged,
                this, &MLSlidingWindowModel<T>::onLocalSizeChanged);
        connect(&*m_cache, &BaseListCache::localDataChanged,
                this, &MLSlidingWindowModel<T>::onLocalDataChanged);

        m_cache->initCount();
    }

    void invalidateCache()
    {
        m_cache.reset();
    }

    T* item(int signedidx) const
    {
        validateCache();

        ssize_t count = m_cache->count();
        if (count == COUNT_UNINITIALIZED || signedidx < 0
                || signedidx >= count)
            return nullptr;

        unsigned int idx = static_cast<unsigned int>(signedidx);
        m_cache->refer(idx);

        const std::unique_ptr<T> *item = m_cache->get(idx);
        if (!item)
            /* Not in cache */
            return nullptr;

        /* Return raw pointer */
        return item->get();
    }

    virtual void onVlcMlEvent(const MLEvent &event) override
    {
        switch (event.i_type)
        {
            case VLC_ML_EVENT_MEDIA_THUMBNAIL_GENERATED:
            {
                if (event.media_thumbnail_generated.b_success) {
                    if (!m_cache)
                        break;

                    ssize_t stotal = m_cache->count();
                    if (stotal == COUNT_UNINITIALIZED)
                        break;

                    size_t total = static_cast<size_t>(stotal);
                    for (size_t i = 0; i < total; ++i)
                    {
                        const std::unique_ptr<T> *item = m_cache->get(i);
                        if (!item)
                            /* Only consider items available locally in cache */
                            break;

                        T *localItem = item->get();
                        if (localItem->getId().id == event.media_thumbnail_generated.i_media_id)
                        {
                            thumbnailUpdated(i);
                            break;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
        MLBaseModel::onVlcMlEvent( event );
    }

    /* Data loader for the cache */
    struct BaseLoader : public ListCacheLoader<std::unique_ptr<T>>
    {
        BaseLoader(vlc_medialibrary_t *ml, MLParentId parent, QString searchPattern,
                   vlc_ml_sorting_criteria_t sort, bool sort_desc)
            : m_ml(ml)
            , m_parent(parent)
            , m_searchPattern(searchPattern)
            , m_sort(sort)
            , m_sort_desc(sort_desc)
        {
        }

        BaseLoader(const MLSlidingWindowModel<T> &model)
            : BaseLoader(model.m_ml, model.m_parent, model.m_search_pattern, model.m_sort, model.m_sort_desc)
        {
        }

        MLQueryParams getParams(size_t index = 0, size_t count = 0) const
        {
            return { m_searchPattern.toUtf8(), m_sort, m_sort_desc, index, count };
        }

    protected:
        vlc_medialibrary_t *m_ml;
        MLParentId m_parent;
        QString m_searchPattern;
        vlc_ml_sorting_criteria_t m_sort;
        bool m_sort_desc;
    };

private:
    virtual void thumbnailUpdated( int ) {}

    mutable std::unique_ptr<ListCache<std::unique_ptr<T>>> m_cache;
};

#endif // MLBASEMODEL_HPP
