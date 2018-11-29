#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include "scene.h"
#include "settings.h"
#include "items/itemfactory.h"
#include "items/item.h"
#include "items/node.h"
#include "items/connector.h"
#include "items/wire.h"
#include "items/wirenet.h"

using namespace QSchematic;

Scene::Scene(QObject* parent) :
    QGraphicsScene(parent),
    _mode(NormalMode),
    _newWireSegment(false),
    _invertWirePosture(false)
{
}

QJsonObject Scene::toJson() const
{
    QJsonObject object;

    // Scene rect
    {
        QJsonObject rectObject;
        const QRect& rect = sceneRect().toRect();
        rectObject.insert("x", rect.x());
        rectObject.insert("y", rect.y());
        rectObject.insert("width", rect.width());
        rectObject.insert("height", rect.height());

        object.insert("scene rect", rectObject);
    }

    // Nodes
    QJsonArray itemsArray;
    for (const Node* node : nodes()) {
        itemsArray.append(node->toJson());
    }
    object.insert("nodes", itemsArray);

    // WireNets
    QJsonArray netsArray;
    for (const WireNet* net : nets()) {
        netsArray.append(net->toJson());
    }
    object.insert("nets", netsArray);

    return object;
}

bool Scene::fromJson(const QJsonObject& object)
{
    Q_UNUSED(object)

    // Scene rect
    {
        const QJsonObject& rectObject = object["scene rect"].toObject();

        QRect sceneRect;
        sceneRect.setX(rectObject["x"].toInt());
        sceneRect.setY(rectObject["y"].toInt());
        sceneRect.setWidth(rectObject["width"].toInt());
        sceneRect.setHeight(rectObject["height"].toInt());
        setSceneRect(sceneRect);
    }

    // Nodes
    {
        QJsonArray array = object["nodes"].toArray();
        for (const QJsonValue& value : array) {
            QJsonObject object = value.toObject();
            if (!object.isEmpty()) {
                std::unique_ptr<Item> node = ItemFactory::instance().fromJson(object);
                node->fromJson(object);
                addItem(node.release());
            }
        }
    }

    // WireNets
    {
        QJsonArray array = object["nets"].toArray();
        for (const QJsonValue& value : array) {
            QJsonObject object = value.toObject();
            if (!object.isEmpty()) {
                std::unique_ptr<WireNet> net(new WireNet);
                net->fromJson(object);

                for (Wire* wire : net->wires()) {
                    addItem(wire);
                }

                addWireNet(std::move(net));
            }
        }
    }

    return true;
}

void Scene::setSettings(const Settings& settings)
{
    // Update settings of all items
    for (Item* item : items()) {
        item->setSettings(settings);
    }

    // Store new settings
    _settings = settings;

    // Redraw
    update();
}

void Scene::setWireFactory(const std::function<std::unique_ptr<Wire>()>& factory)
{
    _wireFactory = factory;
}

void Scene::setMode(Scene::Mode mode)
{
    // Check what the previous mode was
    switch (_mode) {

    // Discard current wire/bus
    case WireMode:
        _newWire.reset(nullptr);
        break;

    default:
        break;

    }

    // Store the new mode
    _mode = mode;

    // Update the UI
    update();

    // Let the world know
    emit modeChanged(_mode);
}
void Scene::toggleWirePosture()
{
    _invertWirePosture = !_invertWirePosture;
}

bool Scene::addItem(Item* item)
{
    setupNewItem(item);

    // Add to scene
    QGraphicsScene::addItem(item);

    return true;
}

QList<Item*> Scene::items() const
{
    QList<Item*> items;

    for (QGraphicsItem* i : QGraphicsScene::items()) {
        Item* item = dynamic_cast<Item*>(i);
        if (!item) {
            continue;
        }

        items << item;
    }

    return items;
}

QList<Item*> Scene::items(Item::ItemType itemType) const
{
    QList<Item*> items;

    for (QGraphicsItem* i : QGraphicsScene::items()) {
        Item* item = dynamic_cast<Item*>(i);
        if (!item or item->type() != itemType) {
            continue;
        }

        items << item;
    }

    return items;
}

QList<Node*> Scene::nodes() const
{
    QList<Node*> nodes;

    for (QGraphicsItem* i : QGraphicsScene::items()) {
        Node* node = dynamic_cast<Node*>(i);
        if (!node) {
            continue;
        }

        nodes << node;
    }

    return nodes;
}

bool Scene::addWire(Wire* wire)
{
    if (!wire)
        return false;

    // Check if any point of the wire lies on any line segment of all existing line segments.
    // If yes, add to that net. Otherwise, create a new one
    for (WireNet* net : _nets) {
        for (const Line& line : net->lineSegments()) {
            for (const WirePoint& point : wire->points()) {
                if (line.containsPoint(point.toPoint(), 0)) {
                    net->addWire(*wire);
                    return true;
                }
            }
        }
    }

    // Check if any line segment of the wire lies on any point of all existing wires.
    // If yes, add to that net. Otherwise, create a new one
    for (WireNet* net : _nets) {
        for (const Wire* otherWire : net->wires()) {
            for (const WirePoint& otherPoint : otherWire->points()) {
                for (const Line& line : wire->lineSegments()) {
                    if (line.containsPoint(otherPoint.toPoint())) {
                        net->addWire(*wire);
                        return true;
                    }
                }
            }
        }
    }

    // No point of the new wire lies on an existing line segment - create a new wire net
    std::unique_ptr<WireNet> newNet(new WireNet);
    newNet->addWire(*wire);
    addWireNet(std::move(newNet));

    return true;
}

bool Scene::removeWire(Wire& wire)
{
    // Remove the wire from the list
    QList<WireNet*> netsToDelete;
    for (WireNet* net : _nets) {
        if (net->contains(wire)) {
            net->removeWire(wire);
        }

        if (net->wires().count() < 1) {
            netsToDelete.append(net);
        }
    }

    // Delete the net if this was the nets last wire
    for (WireNet* net : netsToDelete) {
        _nets.removeAll(net);
        delete net;
    }

    return true;
}

QList<Wire*> Scene::wires() const
{
    QList<Wire*> list;

    for (const WireNet* wireNet : _nets) {
        list.append(wireNet->wires());
    }

    return list;
}

QList<WireNet*> Scene::nets() const
{
    return _nets;
}

QList<WireNet*> Scene::nets(const WireNet& wireNet) const
{
    QList<WireNet*> list;

    for (WireNet* net : _nets) {
        if (!net) {
            continue;
        }

        if (net->name().isEmpty()) {
            continue;
        }

        if (QString::compare(net->name(), wireNet.name(), Qt::CaseInsensitive) == 0) {
            list.append(net);
        }
    }

    return list;
}

WireNet* Scene::net(const Wire& wire) const
{
    for (WireNet* net : _nets) {
        for (const Wire* w : net->wires()) {
            if (w == &wire) {
                return net;
            }
        }
    }

    return nullptr;
}

QList<WireNet*> Scene::netsAt(const QPoint& point)
{
    QList<WireNet*> list;

    for (WireNet* net : _nets) {
        for (const Line& line : net->lineSegments()) {
            if (line.containsPoint(point) && !list.contains(net)) {
                list.append(net);
            }
        }
    }

    return list;
}

void Scene::itemMoved(const Item& item, const QVector2D& movedBy)
{
    // Nothing to do if the item didn't move at all
    if (movedBy.isNull()) {
        return;
    }

    // If this is a Node class, move wires with it
    const Node* node = dynamic_cast<const Node*>(&item);
    if (node) {
        // Create a list of all wires there were connected to the SchematicObject
        QList<Wire*> wiresConnectedToMovingObjects = wiresConnectedTo(*node, movedBy*(-1));

        // Update wire positions
        for (Wire* wire : wiresConnectedToMovingObjects) {
            for (const QPoint& connectionPoint : node->connectionPoints()) {
                wireMovePoint(connectionPoint, *wire, movedBy);
            }
        }

        // Clean up the wires
        for (const Wire* wire : wiresConnectedToMovingObjects) {
            WireNet* wireNet = net(*wire);
            if (!wireNet) {
                continue;
            }

            wireNet->removeDuplicatePoints();
            wireNet->removeObsoletePoints();
        }
    }
}

void Scene::wireNetHighlightChanged(bool highlighted)
{
    WireNet* wireNet = qobject_cast<WireNet*>(sender());
    if (!wireNet)
        return;

    // Highlight all wire nets that are part of this net
    for (WireNet* otherWireNet : nets(*wireNet)) {
        if (otherWireNet == wireNet) {
            continue;
        }

        otherWireNet->blockSignals(true);
        otherWireNet->setHighlighted(highlighted);
        otherWireNet->blockSignals(false);
    }
}

void Scene::wirePointMoved(Wire& wire, WirePoint& point)
{
    Q_UNUSED(point)

    // Remove the wire from the current net
    // Remove wire nets that have no more wires
    QList<WireNet*> netsToDelete;
    for (WireNet* net : _nets) {
        if (net->contains(wire)) {
            net->removeWire(wire);
            net->setHighlighted(false); // Clear the highlighting
            break;
        }

        if (net->wires().count() <= 0) {
            netsToDelete.append(net);
        }
    }
    for (WireNet* net : netsToDelete) {
        _nets.removeAll(net);
        delete net;
    }

    // If the point is on an existing wire net, add the wire to that net. Otherwise, create a new net
    addWire(&wire);
}

void Scene::wireMovePoint(const QPoint& point, Wire& wire, const QVector2D& movedBy) const
{
    // If there are only two points (one line segment) and we are supposed to preserve
    // straight angles, we need to insert two additional points if we are not moving in
    // the direction of the line.
    if (wire.points().count() == 2 && _settings.preserveStraightAngles) {
        const Line& line = wire.lineSegments().first();

        // Only do this if we're not moving in the direction of the line. Because in that case
        // this is unnecessary as we're just moving one of the two points.
        if ((line.isHorizontal() && !qFuzzyIsNull(movedBy.y())) || (line.isVertical() && !qFuzzyIsNull(movedBy.x()))) {
            qreal lineLength = line.lenght();
            QPoint p;

            // The line is horizontal
            if (line.isHorizontal()) {
                QPoint leftPoint = line.p1();
                if (line.p2().x() < line.p1().x()) {
                    leftPoint = line.p2();
                }

                p.rx() = leftPoint.x() + static_cast<int>(lineLength/2);
                p.ry() = leftPoint.y();

            // The line is vertical
            } else {
                QPoint upperPoint = line.p1();
                if (line.p2().x() < line.p1().x()) {
                    upperPoint = line.p2();
                }

                p.rx() = upperPoint.x();
                p.ry() = upperPoint.y() + static_cast<int>(lineLength/2);
            }

            // Insert twice as these two points will form the new additional vertical or
            // horizontal line segment that is required to preserver straight angles.
            wire.insertPoint(1, p);
            wire.insertPoint(1, p);
        }
    }

    // Move the points
    for (int i = 0; i < wire.points().count(); i++) {
        QPoint currPoint = wire.points().at(i);
        if (currPoint == point-movedBy.toPoint()) {

            // Preserve straight angles (if supposed to)
            if (_settings.preserveStraightAngles) {

                // Move previous point
                if (i >= 1) {
                    QPoint prevPoint = wire.points().at(i-1);
                    Line line(prevPoint, currPoint);

                    // Make sure that two wire points never collide
                    if (i >= 2 && Line(currPoint+movedBy.toPoint(), prevPoint).lenght() <= 2) {
                        wire.moveLineSegmentBy(i-2, movedBy);
                    }

                    // The line is horizontal
                    if (line.isHorizontal()) {
                        wire.movePointBy(i-1, QVector2D(0, movedBy.y()));
                    }

                    // The line is vertical
                    else if (line.isVertical()) {
                        wire.movePointBy(i-1, QVector2D(movedBy.x(), 0));
                    }
                }

                // Move next point
                if (i < wire.points().count()-1) {
                    QPoint nextPoint = wire.points().at(i+1);
                    Line line(currPoint, nextPoint);

                    // Make sure that two wire points never collide
                    if (Line(currPoint+movedBy.toPoint(), nextPoint).lenght() <= 2) {
                        wire.moveLineSegmentBy(i+1, movedBy);
                    }

                    // The line is horizontal
                    if (line.isHorizontal()) {
                        wire.movePointBy(i+1, QVector2D(0, movedBy.y()));
                    }

                    // The line is vertical
                    else if (line.isVertical()) {
                        wire.movePointBy(i+1, QVector2D(movedBy.x(), 0));
                    }
                }
            }

            // Move the actual point itself
            wire.movePointBy(i, movedBy);

            break;
        }
    }
}

QList<Wire*> Scene::wiresConnectedTo(const Node& node, const QVector2D& offset) const
{
    QList<Wire*> list;

    for (Wire* wire : wires()) {
        for (const WirePoint& wirePoint : wire->points()) {
            for (const QPoint& connectionPoint : node.connectionPoints()) {
                if (wirePoint.toPoint() == connectionPoint+offset.toPoint()) {
                    list.append(wire);
                    break;
                }
            }
        }
    }

    return list;
}

void Scene::showPopup(const Item& item)
{
    _popupInfobox.reset(addWidget(item.popupInfobox()));

    if (_popupInfobox) {
        _popupInfobox->setPos(_lastMousePos + QPointF(5, 5));
        _popupInfobox->setZValue(100);
    }
}

void Scene::addWireNet(std::unique_ptr<WireNet> wireNet)
{
    if (!wireNet) {
        return;
    }

    // Take ownership
    WireNet* net = wireNet.release();

    connect(net, &WireNet::pointMoved, this, &Scene::wirePointMoved);
    connect(net, &WireNet::highlightChanged, this, &Scene::wireNetHighlightChanged);

    _nets.append(net);

    update();
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();

    switch (_mode) {
    case NormalMode:
    {
        // Reset stuff
        _newWire.reset();

        // Handle selection stuff internally (do not remove this!)
        QGraphicsScene::mousePressEvent(event);

        return;
    }

    case WireMode:
    {

        // Left mouse button
        if (event->button() == Qt::LeftButton) {

            QPoint mousetoGridPoint = _settings.toGridPoint(event->scenePos());

            // Start a new wire if there isn't already one. Else continue the current one.
            if (!_newWire) {
                if (_wireFactory) {
                    _newWire.reset(_wireFactory().release());
                } else {
                    _newWire.reset(new Wire);
                }
                _newWire->setAcceptHoverEvents(false);
                addItem(_newWire.data());
            }
            _newWire->appendPoint(mousetoGridPoint);
            _newWireSegment = true;

            return;

        }

        return;
    }

    }
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();

    switch (_mode) {
    case NormalMode:
    {
        // Handle stuff like mouse movement (do not remove this!)
        QGraphicsScene::mouseReleaseEvent(event);

        break;
    }

    case WireMode:
    {
        // Right mouse button
        if (event->button() == Qt::RightButton) {

            // Change the mode back to NormalMode if nothing below cursor
            if (QGraphicsScene::items(event->scenePos()).isEmpty()) {
                setMode(NormalMode);
            }

            // Show the context menu stuff
            QGraphicsScene::mouseReleaseEvent(event);
        }

        break;
    }
    }
}

void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    // Get rid of any popup infobox
    _popupInfobox.reset();

    // Retrieve the new mouse position
    QPointF newMousePos = event->scenePos();

    switch (_mode) {
    case NormalMode:

        // Perform the move
        QGraphicsScene::mouseMoveEvent(event);

        break;

    case WireMode:
    {
        // Make sure that there's a wire
        if (!_newWire) {
            break;
        }

        // Transform mouse coordinates to grid positions (snapped to nearest grid point)
        QPoint mouseGridPos = _settings.toGridPoint(newMousePos);

        // Add a new wire segment. Only allow straight angles (if supposed to)
        if (_settings.routeStraightAngles) {
            if (_newWireSegment) {
                // Remove the last point if there was a previous segment
                if (_newWire->points().count() > 1) {
                    _newWire->removeLastPoint();
                }

                // Create the intermediate point that creates the straight angle
                WirePoint prevNode(_newWire->points().at(_newWire->points().count()-1));
                QPoint corner(prevNode.x(), mouseGridPos.y());
                if (_invertWirePosture) {
                    corner.setX(mouseGridPos.x());
                    corner.setY(prevNode.y());
                }

                // Add the two new points
                _newWire->appendPoint(corner);
                _newWire->appendPoint(mouseGridPos);

                _newWireSegment = false;
            } else {
                // Create the intermediate point that creates the straight angle
                WirePoint prevNode(_newWire->points().at(_newWire->points().count()-3));
                QPoint corner(prevNode.x(), mouseGridPos.y());
                if (_invertWirePosture) {
                    corner.setX(mouseGridPos.x());
                    corner.setY(prevNode.y());
                }

                // Modify the actual wire
                _newWire->movePointTo(_newWire->points().count()-2, corner);
                _newWire->movePointTo(_newWire->points().count()-1, mouseGridPos);
            }
        } else {
            // Don't care about angles and stuff. Fuck geometry, right?
            QPoint newGridPoint = _settings.toGridPoint(newMousePos);
            if (_newWire->points().count() > 1) {
                _newWire->movePointTo(_newWire->points().count()-1, newGridPoint);
            } else {
                _newWire->appendPoint(newGridPoint);
            }
        }

        break;
    }

    }

    // Save the last mouse position
    _lastMousePos = newMousePos;
}

void Scene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();

    switch (_mode) {
    case NormalMode:
    {
        return;
    }

    case WireMode:
    {

        // Only do something if there's a wire
        if (_newWire && _newWire->points().count() > 1) {
            bool wireIsFloating = true;

            // Get rid of the last point as mouseDoubleClickEvent() is following mousePressEvent()
            _newWire->removeLastPoint();

            // Check whether the wire was connected to a connector
            for (const QPoint& connectionPoint : connectionPoints()) {
                if (connectionPoint == _newWire->points().last()) {
                    wireIsFloating = false;
                    break;
                }
            }

            // Check wether the wire was connected to another wire
            for (const Wire* wire : wires()) {
                if (wire->pointIsOnWire(_newWire->points().last())) {
                    wireIsFloating = false;
                    break;
                }
            }

            // Notify the user if the wire ended up on a non-valid thingy
            if (wireIsFloating) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Wire mode");
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText("A wire must end on either:\n"
                               "  + A node connector\n"
                               "  + A wire\n");
                msgBox.exec();

                _newWire->removeLastPoint();

                return;
            }

            // Finish the current wire
            _newWire->setAcceptHoverEvents(true);
            _newWire->setFlag(QGraphicsItem::ItemIsSelectable, true);
            _newWire->removeDuplicatePoints();
            connect(_newWire.data(), &Wire::showPopup, this, &Scene::showPopup);
            addWire(_newWire.take());

            return;
        }

        return;
    }

    }
}

void Scene::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Background pen
    QPen backgroundPen;
    backgroundPen.setStyle(Qt::NoPen);

    // Background brush
    QBrush backgroundBrush;
    backgroundBrush.setStyle(Qt::SolidPattern);
    backgroundBrush.setColor(Qt::white);

    // Grid pen
    QPen gridPen;
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setColor(Qt::gray);
    gridPen.setCapStyle(Qt::RoundCap);
    gridPen.setWidth(_settings.gridPointSize);

    // Grid brush
    QBrush gridBrush;
    gridBrush.setStyle(Qt::NoBrush);

    // Draw background
    painter->setPen(backgroundPen);
    painter->setBrush(backgroundBrush);
    painter->drawRect(rect);

    // Draw the grid if supposed to
    if (_settings.showGrid && (_settings.gridSize > 0)) {
        qreal left = int(rect.left()) - (int(rect.left()) % _settings.gridSize);
        qreal top = int(rect.top()) - (int(rect.top()) % _settings.gridSize);

        // Create a list of points
        QVector<QPointF> points;
        for (qreal x = left; x < rect.right(); x += _settings.gridSize) {
            for (qreal y = top; y < rect.bottom(); y += _settings.gridSize) {
                points.append(QPointF(x,y));
            }
        }

        // Draw the actual grid points
        painter->setPen(gridPen);
        painter->setBrush(gridBrush);
        painter->drawPoints(points.data(), points.size());
    }
}

void Scene::setupNewItem(Item* item)
{
    // Set settings
    item->setSettings(_settings);

    // Connections
    connect(item, &Item::moved, this, &Scene::itemMoved);
    connect(item, &Item::showPopup, this, &Scene::showPopup);
}

QList<QPoint> Scene::connectionPoints() const
{
    QList<QPoint> list;

    for (const Node* node : nodes()) {
        list << node->connectionPoints();
    }

    return list;
}
