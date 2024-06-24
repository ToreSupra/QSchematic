#include "operationdemo1.hpp"
#include "operationconnector.hpp"
#include "itemtypes.hpp"

#include <qschematic/items/label.hpp>

struct ConnectorAttribute {
    QPoint point;
    QString name;
};

OperationDemo1::OperationDemo1(QGraphicsItem* parent) :
    Operation(::ItemType::OperationDemo1Type, parent)
{
    setSize(160, 160);
    label()->setText(QStringLiteral("Demo 1"));

    QVector<ConnectorAttribute> connectorAttributes = {
        { QPoint(0, 2), QStringLiteral("in 1") },
        { QPoint(0, 4), QStringLiteral("in 2") },
        { QPoint(0, 6), QStringLiteral("in 3") },
        { QPoint(8, 4), QStringLiteral("out") }
    };

    for (const auto& c : connectorAttributes) {
        auto connector = std::make_shared<OperationConnector>(c.point, c.name);
        connector->label()->setVisible(true);
        addConnector(connector);
    }
}

BOOST_CLASS_EXPORT_IMPLEMENT(OperationDemo1)

template<class Archive>
void OperationDemo1::serialize(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version)

    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Operation);
}

std::shared_ptr<QSchematic::Items::Item> OperationDemo1::deepCopy() const
{
    auto clone = std::make_shared<OperationDemo1>(parentItem());
    copyAttributes(*clone);

    return clone;
}

void OperationDemo1::copyAttributes(OperationDemo1& dest) const
{
    Operation::copyAttributes(dest);
}
