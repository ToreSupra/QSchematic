#pragma once

#include "itemfunctions.hpp"
#include "../types.hpp"
#include "../settings.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <QDebug>
#include <QGraphicsObject>
#include <QWidget>

#include <memory>

namespace QSchematic
{
    class Scene;
}

namespace QSchematic::Items
{

    class Item :
        public QGraphicsObject,
        public std::enable_shared_from_this<Item>
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Item)

    public:
        enum ItemType {
            NodeType               = QGraphicsItem::UserType + 1,
            WireType,
            WireRoundedCornersType,
            ConnectorType,
            LabelType,
            SplineWireType,
            BackgroundType,

            QSchematicItemUserType = QGraphicsItem::UserType + 100
        };
        Q_ENUM(ItemType)

        explicit
        Item(int type, QGraphicsItem* parent = nullptr);

        ~Item() override;

        /**
         * These funcs should be the only source for obtaining a canonical
         * shared-/weak-ptr to the item. It _must_ be allocated with make_shared
         * or shared-constructor — ,no compile time check validates that.
         * For convenience it's also possible to cast by simply explicitly
         * passing a template arg
         */
        /// @{
        template <typename RetT = Item>
        auto sharedPtr() const -> std::shared_ptr<const RetT>
        {
            if constexpr (std::is_same_v<RetT, Item>)
                return shared_from_this();
            else
                return std::dynamic_pointer_cast<const RetT>(shared_from_this());
        }

        template <typename RetT = Item>
        auto sharedPtr() -> std::shared_ptr<RetT>
        {
            if constexpr (std::is_same_v<RetT, Item>)
                return shared_from_this();
            else
                return std::dynamic_pointer_cast<RetT>(shared_from_this());
        }

        template <typename RetT = Item>
        auto weakPtr() const -> std::weak_ptr<const RetT>
        {
            if constexpr (std::is_same_v<RetT, Item>)
                return weak_from_this();
            else
                return std::dynamic_pointer_cast<const RetT>(weak_from_this());
        }

        template <typename RetT = Item>
        auto weakPtr() -> std::weak_ptr<RetT>
        {
            if constexpr (std::is_same_v<RetT, Item>)
                return weak_from_this();
            else
                return std::dynamic_pointer_cast<RetT>(weak_from_this());
        }
        /// @}

        friend class ::boost::serialization::access;
        template<class Archive>
        void save(Archive& ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive& ar, const unsigned int version);
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        [[nodiscard]]
        virtual
        std::shared_ptr<Item>
        deepCopy() const = 0;

        // QGraphicsObject overload
        [[nodiscard]]
        int
        type() const final;

        void
        setGridPos(const QPoint& gridPos);

        void
        setGridPos(int x, int y);

        void
        setGridPosX(int x);

        void
        setGridPosY(int y);

        [[nodiscard]]
        QPoint
        gridPos() const;

        [[nodiscard]]
        int
        gridPosX() const;

        [[nodiscard]]
        int
        gridPosY() const;

        void
        setPos(const QPointF& pos);

        void
        setPos(qreal x, qreal y);

        void
        setPosX(qreal x);

        void
        setPosY(qreal y);

        [[nodiscard]]
        QPointF
        pos() const;

        [[nodiscard]]
        qreal
        posX() const;

        [[nodiscard]]
        qreal
        posY() const;

        void
        setScenePos(const QPointF& point);

        void
        setScenePos(qreal x, qreal y);

        void
        setScenePosX(qreal x);

        void
        setScenePosY(qreal y);

        [[nodiscard]]
        QPointF
        scenePos() const;

        [[nodiscard]]
        qreal
        scenePosX() const;

        [[nodiscard]]
        qreal
        scenePosY() const;

        void
        moveBy(const QVector2D& moveBy);

        void
        setSettings(const Settings& settings);

        [[nodiscard]]
        const Settings&
        settings() const;

        void
        setMovable(bool enabled);

        [[nodiscard]]
        bool
        isMovable() const;

        void
        setSnapToGrid(bool enabled);

        [[nodiscard]]
        bool
        snapToGrid() const;

        void
        setHighlighted(bool isHighlighted);

        void
        setHighlightEnabled(bool enabled);

        [[nodiscard]]
        bool
        highlightEnabled() const;

        [[nodiscard]]
        QPixmap
        toPixmap(QPointF& hotSpot, qreal scale = 1.0);

        virtual
        void
        update();

        [[nodiscard]]
        Scene*
        scene() const;

        [[nodiscard]]
        virtual
        std::unique_ptr<QWidget>
        popup() const
        {
            return nullptr;
        }

    Q_SIGNALS:
        void moved(Item& item, const QVector2D& movedBy);
        void movedInScene(Item& item);
        void rotated(Item& item, qreal rotation);
        void highlightChanged(const Item& item, bool isHighlighted);
        void settingsChanged();

    protected:
        Settings _settings;

        void
        copyAttributes(Item& dest) const;

        [[nodiscard]]
        bool
        isHighlighted() const;

        QVariant
        itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

    private Q_SLOTS:
        void posChanged();
        void scenePosChanged();
        void rotChanged();

    private:
        int _type;
        bool _snapToGrid;
        bool _highlightEnabled;
        bool _highlighted;
        QPointF _oldPos;
        qreal _oldRot;
    };

    BOOST_SERIALIZATION_ASSUME_ABSTRACT(Item)

}
