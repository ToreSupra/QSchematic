#pragma once

#include "rectitem.hpp"
#include "connector.hpp"
#include "../types.hpp"

#include <QList>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;

namespace QSchematic::Items
{

    class Connector;

    class Node :
        public RectItem
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Node)

    public:
        Node(int type = Item::NodeType, QGraphicsItem* parent = nullptr);
        ~Node() override;

        friend class ::boost::serialization::access;
        template<class Archive>
        void save(Archive& ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive& ar, const unsigned int version);
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        std::shared_ptr<Item> deepCopy() const override;

        bool addConnector(const std::shared_ptr<Connector>& connector);
        bool removeConnector(const std::shared_ptr<Connector>& connector);
        void clearConnectors();
        QList<std::shared_ptr<Connector>> connectors() const;
        QList<QPointF> connectionPointsRelative() const;
        QList<QPointF> connectionPointsAbsolute() const;
        void setConnectorsMovable(bool enabled);
        bool connectorsMovable() const;
        void setConnectorsSnapPolicy(Connector::SnapPolicy policy);
        Connector::SnapPolicy connectorsSnapPolicy() const;
        void setConnectorsSnapToGrid(bool enabled);
        bool connectorsSnapToGrid() const;
        void alignConnectorLabels() const;

        void sizeChangedEvent(QSizeF oldSize, QSizeF newSize) override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        void update() override;

    protected:
        void copyAttributes(Node& dest) const;
        void addSpecialConnector(const std::shared_ptr<Connector>& connectors);

    private:
        void propagateSettings();

        bool _connectorsMovable;
        Connector::SnapPolicy _connectorsSnapPolicy;
        bool _connectorsSnapToGrid;
        QList<std::shared_ptr<Connector>> _connectors;
        QList<std::shared_ptr<Connector>> _specialConnectors;  // Ignored in serialization and deep-copy
    };

}

BOOST_CLASS_EXPORT_KEY(QSchematic::Items::Node)
