#pragma once

#include <QFileInfo>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>

namespace paw {

// QFileSystemModel's QDir::Hidden filter hides dot-files only on Unix:
// on Windows a leading dot does not make a file hidden, so ".env" would
// stay visible no matter what the settings say. This proxy adds the
// platform-independent part: when hidden files are disabled, entries whose
// name starts with a dot and files carrying the OS hidden attribute
// (QFileInfo::isHidden) are filtered out.
class HiddenFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit HiddenFilterProxy(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setShowHidden(bool show)
    {
        if (m_show == show)
            return;
        m_show = show;
        invalidateRowsFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (m_show)
            return true;
        const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (idx.data(QFileSystemModel::FileNameRole).toString().startsWith(QLatin1Char('.')))
            return false;
        return !QFileInfo(idx.data(QFileSystemModel::FilePathRole).toString()).isHidden();
    }

    // Directories first, then case-insensitive by name — same order
    // QFileSystemModel uses when it sorts without a proxy.
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        const QFileInfo l(left.data(QFileSystemModel::FilePathRole).toString());
        const QFileInfo r(right.data(QFileSystemModel::FilePathRole).toString());
        if (l.isDir() != r.isDir())
            return l.isDir();
        return QString::compare(left.data(Qt::DisplayRole).toString(),
                                right.data(Qt::DisplayRole).toString(),
                                Qt::CaseInsensitive) < 0;
    }

private:
    bool m_show = true;
};

} // namespace paw
