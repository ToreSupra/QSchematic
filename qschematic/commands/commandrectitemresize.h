#pragma once

#include "commandbase.h"

#include <QPointer>
#include <QPoint>
#include <QSize>

namespace QSchematic
{
    class RectItem;
}

namespace QSchematic::Commands
{

    class RectItemResize :
        public Base
    {
    public:
        RectItemResize(QPointer<RectItem> item, const QPointF& newPos, const QSizeF& newSize, QUndoCommand* parent = nullptr);

        int id() const override;
        bool mergeWith(const QUndoCommand* command) override;
        void undo() override;
        void redo() override;

    private:
        QPointer<RectItem> _item;
        QPointF _oldPos;
        QPointF _newPos;
        QSizeF _oldSize;
        QSizeF _newSize;
    };

}
