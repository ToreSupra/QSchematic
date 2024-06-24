#include "node.hpp"
#include "../utils.hpp"
#include "../scene.hpp"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QtMath>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

const QColor COLOR_HIGHLIGHTED = QColor(Qt::blue);
const QColor COLOR_BODY_FILL   = QColor(Qt::green);
const QColor COLOR_BODY_BORDER = QColor(Qt::black);
const qreal PEN_WIDTH          = 1.5;

BOOST_CLASS_EXPORT_IMPLEMENT(QSchematic::Items::Node)

using namespace QSchematic::Items;

const int DEFAULT_WIDTH     = 160;
const int DEFAULT_HEIGHT    = 240;

Node::Node(int type, QGraphicsItem* parent) :
    RectItem(type, parent),
    _connectorsMovable(false),
    _connectorsSnapPolicy(Connector::NodeSizerectOutline),
    _connectorsSnapToGrid(true)
{
    connect(this, &Node::settingsChanged, this, &Node::propagateSettings);
}

Node::~Node()
{
    dissociate_items(_connectors);
    dissociate_items(_specialConnectors);
}

template void Node::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar, const unsigned int version);
template void Node::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive& ar, const unsigned int version);
template void Node::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive& ar, const unsigned int version);
template void Node::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive& ar, const unsigned int version);

template<class Archive>
void Node::save(Archive& ar, const unsigned int version) const
{
    Q_UNUSED(version)

    // Connectors configuration
    ar& boost::serialization::make_nvp("connectors_movable", _connectorsMovable);
    int sp = static_cast<int>(connectorsSnapPolicy());
    ar& boost::serialization::make_nvp("connectors_snap_policy", sp);
    bool sg = connectorsSnapToGrid();
    ar& boost::serialization::make_nvp("connectors_snap_to_grid", sg);

    // Connectors
    auto cs = connectors();
    std::vector<std::shared_ptr<Connector>> notSpecialConnectors;
    notSpecialConnectors.reserve(cs.size());
    for (const auto& connector : cs) {
        if (!_specialConnectors.contains(connector)) {
            notSpecialConnectors.push_back(connector);
        }
    }
    ar & boost::serialization::make_nvp("connectors", notSpecialConnectors);

    // Root
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectItem);
    qreal w, h;
    w = size().width();
    ar& boost::serialization::make_nvp("width", w);
    h = size().height();
    ar& boost::serialization::make_nvp("height", h);
    bool amr = allowMouseResize();
    ar& boost::serialization::make_nvp("allow_mouse_resize", amr);
    bool amrot = allowMouseRotate();
    ar& boost::serialization::make_nvp("allow_mouse_rotate", amrot);
}

template<class Archive>
void Node::load(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version)

    // Connectors configuration
    bool connectorsMovable;
    Connector::SnapPolicy connectorsSnapPolicy;
    bool connectorsSnapToGrid;
    ar & boost::serialization::make_nvp("connectors_movable", connectorsMovable);
    ar & boost::serialization::make_nvp("connectors_snap_policy", connectorsSnapPolicy);
    ar & boost::serialization::make_nvp("connectors_snap_to_grid", connectorsSnapToGrid);
    setConnectorsMovable(connectorsMovable);
    setConnectorsSnapPolicy(connectorsSnapPolicy);
    setConnectorsSnapToGrid(connectorsSnapToGrid);

    // Connectors
    clearConnectors();
    std::vector<std::shared_ptr<Connector>> notSpecialConnectors;
    ar & boost::serialization::make_nvp("connectors", notSpecialConnectors);
    for (const auto& connector : notSpecialConnectors) {
        addConnector(connector);
    }

    // Root
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectItem);
    qreal width, height;
    ar & boost::serialization::make_nvp("width", width);
    ar & boost::serialization::make_nvp("height", height);
    setSize(width, height);
    bool allowMouseResize, allowMouseRotate;
    ar & boost::serialization::make_nvp("allow_mouse_resize", allowMouseResize);
    setAllowMouseResize(allowMouseResize);
    ar & boost::serialization::make_nvp("allow_mouse_rotate", allowMouseRotate);
    setAllowMouseRotate(allowMouseRotate);
}

std::shared_ptr<Item> Node::deepCopy() const
{
    auto clone = std::make_shared<Node>(type(), parentItem());
    copyAttributes(*clone);

    return clone;
}

void Node::copyAttributes(Node& dest) const
{
    // Base class
    RectItem::copyAttributes(dest);

    // Connectors
    dest.clearConnectors();
    for (const auto& connector : _connectors) {
        if ( _specialConnectors.contains( connector ) ) {
            continue;
        }

        auto connectorClone = std::dynamic_pointer_cast<Connector>(connector->deepCopy());
        connectorClone->setParentItem(&dest);
        dest._connectors << connectorClone;
    }

    // Attributes
    dest._connectorsMovable = _connectorsMovable;
    dest._connectorsSnapPolicy = _connectorsSnapPolicy;
    dest._connectorsSnapToGrid = _connectorsSnapToGrid;
    dest._specialConnectors = _specialConnectors;
}

void Node::addSpecialConnector(const std::shared_ptr<Connector>& connector)
{
    // Sanity check
    if (!connector)
        return;

    _specialConnectors.push_back( connector );

    addConnector( connector );
}

bool Node::addConnector(const std::shared_ptr<Connector>& connector)
{
    if (!connector) {
        return false;
    }

    connector->setParentItem(this);
    connector->setMovable(_connectorsMovable);
    connector->setSnapPolicy(_connectorsSnapPolicy);
    connector->setSnapToGrid(_connectorsSnapToGrid);
    connector->setSettings(_settings);

    _connectors << connector;

    return true;
}

bool Node::removeConnector(const std::shared_ptr<Connector>& connector)
{
    if (!connector) {
        return false;
    }
    if (!_connectors.contains(connector) && !_specialConnectors.contains(connector)) {
        return false;
    }

    connector->setParentItem(nullptr);

    _connectors.removeAll(connector);
    _specialConnectors.removeAll(connector);

    return true;
}

void Node::clearConnectors()
{
    // Remove from scene
    auto s = scene();
    if (s) {
        for (auto connector : _connectors) {
            s->removeItem(connector);
        }
    }

    // Clear the local list
    _connectors.clear();
}

QList<std::shared_ptr<Connector>> Node::connectors() const
{
    return _connectors;
}

QList<QPointF> Node::connectionPointsRelative() const
{
    QList<QPointF> list;

    for (const auto& connector : _connectors) {
        // Ignore hidden connectors
        if (!connector->isVisible())
            continue;

        // Rotate the position around to the node's origin
        QPointF pos = connector->pos();
        {
            QPointF d = transformOriginPoint() - pos;
            qreal angle = rotation() * M_PI / 180;
            QPointF rotated;
            rotated.setX(qCos(angle) * d.rx() - qSin(angle) * d.ry());
            rotated.setY(qSin(angle) * d.rx() + qCos(angle) * d.ry());
            pos = transformOriginPoint() - rotated;
        }
        list << connector->connectionPoint() + pos;
    }

    return list;
}

QList<QPointF> Node::connectionPointsAbsolute() const
{
    QList<QPointF> list(connectionPointsRelative());

    for (QPointF& point : list) {
        point += pos();
    }

    return list;
}

void Node::setConnectorsMovable(bool enabled)
{
    // Update connectors
    for (auto connector : _connectors) {
        connector->setMovable(enabled);
    }

    // Update local
    _connectorsMovable = enabled;
}

bool Node::connectorsMovable() const
{
    return _connectorsMovable;
}

void Node::setConnectorsSnapPolicy(Connector::SnapPolicy policy)
{
    // Update connectors
    for (auto connector : _connectors) {
        connector->setSnapPolicy(policy);
    }

    // Update local
    _connectorsSnapPolicy = policy;
}

Connector::SnapPolicy Node::connectorsSnapPolicy() const
{
    return _connectorsSnapPolicy;
}

void Node::setConnectorsSnapToGrid(bool enabled)
{
    // Update connectors
    for (auto connector : _connectors) {
        connector->setSnapToGrid(enabled);
    }

    // Update local
    _connectorsSnapToGrid = enabled;
}

bool Node::connectorsSnapToGrid() const
{
    return _connectorsSnapToGrid;
}

void Node::alignConnectorLabels() const
{
    for (auto connector : _connectors) {
        Q_ASSERT(connector);
        connector->alignLabel();
    }
}

void Node::sizeChangedEvent(const QSizeF oldSize, const QSizeF newSize)
{
    for (const auto& connector : connectors()) {
        if (qFuzzyCompare(connector->posX(), oldSize.width()) || connector->posX() > newSize.width())
            connector->setX(newSize.width());

        if (qFuzzyCompare(connector->posY(), oldSize.height()) || connector->posY() > newSize.height())
            connector->setY(newSize.height());
    }
}

void Node::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Draw the bounding rect if debug mode is enabled
    if (_settings.debug) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(Qt::red));
        painter->drawRect(boundingRect());
    }

    // Highlight rectangle
    if (isHighlighted()) {
        // Highlight pen
        QPen highlightPen;
        highlightPen.setStyle(Qt::NoPen);

        // Highlight brush
        QBrush highlightBrush;
        highlightBrush.setStyle(Qt::SolidPattern);
        highlightBrush.setColor(COLOR_HIGHLIGHTED);

        // Highlight rectangle
        painter->setPen(highlightPen);
        painter->setBrush(highlightBrush);
        painter->setOpacity(0.5);
        int adj = _settings.highlightRectPadding;
        painter->drawRoundedRect(sizeRect().adjusted(-adj, -adj, adj, adj), _settings.gridSize/2, _settings.gridSize/2);
    }

    painter->setOpacity(1.0);

    // Body pen
    QPen bodyPen;
    bodyPen.setWidthF(PEN_WIDTH);
    bodyPen.setStyle(Qt::SolidLine);
    bodyPen.setColor(COLOR_BODY_BORDER);

    // Body brush
    QBrush bodyBrush;
    bodyBrush.setStyle(Qt::SolidPattern);
    bodyBrush.setColor(COLOR_BODY_FILL);

    // Draw the component body
    painter->setPen(bodyPen);
    painter->setBrush(bodyBrush);
    painter->drawRoundedRect(sizeRect(), _settings.gridSize/2, _settings.gridSize/2);

    // Resize handles
    if (isSelected() && allowMouseResize()) {
        paintResizeHandles(*painter);
    }

    // Rotate handle
    if (isSelected() && allowMouseRotate()) {
        paintRotateHandle(*painter);
    }
}

void Node::update()
{
    // The item class sets the origin to the center of the bounding box
    // but in this case we want to rotate around the center of the body
    setTransformOriginPoint(sizeRect().center());
    QGraphicsObject::update();
}

void Node::propagateSettings()
{
    for (const auto& connector : connectors()) {
        connector->setSettings(_settings);
    }
}
