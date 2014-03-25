#include "cdtclassification.h"
#include "cdtsegmentationlayer.h"
#include "cdtimagelayer.h"
#include <QList>
#include <QMenu>
#include <QInputDialog>
#include <QtXml>
#include <qgsvectorlayer.h>
#include <qgsmaplayerregistry.h>
#include <qgssinglesymbolrendererv2.h>
#include <qgsrendererv2widget.h>
#include <qgsfillsymbollayerv2.h>

CDTSegmentationLayer::CDTSegmentationLayer(QString imagePath,QObject *parent)
    :CDTBaseObject(parent),
      m_imagePath(imagePath),
      addClassifications(new QAction(tr("Add Classification"),this)),
      actionRemoveSegmentation(new QAction(tr("Remove Segmentation"),this)),
      actionRemoveAllClassifications(new QAction(tr("Remove All Classifications"),this)),
      actionRename(new QAction(tr("Rename Segmentation Name"),this)),
      actionSetLayerProperty(new QAction(tr("Set vector layer property"),this))
{
    keyItem   = new CDTProjectTreeItem(CDTProjectTreeItem::SEGMENTION,CDTProjectTreeItem::VECTOR,QString(),this);
    valueItem = new CDTProjectTreeItem(CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,QString(),this);

    shapefileItem = new CDTProjectTreeItem(CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,QString(),this);
    keyItem->appendRow(
                QList<QStandardItem*>()
                <<new CDTProjectTreeItem(CDTProjectTreeItem::PARAM,CDTProjectTreeItem::EMPTY,tr("Shapefile path"),this)
                <<shapefileItem);

    markfileItem = new CDTProjectTreeItem(CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,QString(),this);
    keyItem->appendRow(
                QList<QStandardItem*>()
                <<new CDTProjectTreeItem(CDTProjectTreeItem::PARAM,CDTProjectTreeItem::EMPTY,tr("Markfile path"),this)
                <<markfileItem);
    paramRootItem = new CDTProjectTreeItem(CDTProjectTreeItem::METHOD_PARAMS,CDTProjectTreeItem::EMPTY,tr("Method"),this);
    paramRootValueItem = new CDTProjectTreeItem(CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,QString(),this);
    keyItem->appendRow(QList<QStandardItem*>()<<paramRootItem<<paramRootValueItem);

    connect(this,SIGNAL(markfilePathChanged()),this,SIGNAL(segmentationChanged()));
    connect(this,SIGNAL(nameChanged()),this,SIGNAL(segmentationChanged()));
    connect(this,SIGNAL(shapefilePathChanged()),this,SIGNAL(segmentationChanged()));
    connect(this,SIGNAL(methodParamsChanged()),this,SIGNAL(segmentationChanged()));
    connect(this,SIGNAL(removeSegmentation(CDTSegmentationLayer*)),(CDTImageLayer*)(this->parent()),SLOT(removeSegmentation(CDTSegmentationLayer*)));
    connect(this,SIGNAL(segmentationChanged()),(CDTImageLayer*)(this->parent()),SIGNAL(imageLayerChanged()));

    connect(addClassifications,SIGNAL(triggered()),this,SLOT(addClassification()));
    connect(actionRemoveSegmentation,SIGNAL(triggered()),this,SLOT(remove()));
    connect(actionRemoveAllClassifications,SIGNAL(triggered()),this,SLOT(removeAllClassifications()));
    connect(actionRename,SIGNAL(triggered()),this,SLOT(onActionRename()));
    connect(actionSetLayerProperty,SIGNAL(triggered()),this,SLOT(onVectorLayerProperty()));
}

void CDTSegmentationLayer::addClassification(CDTClassification *classification)
{
    classifications.push_back(classification);
    emit methodParamsChanged();
    connect(classification,SIGNAL(methodParamsChanged()),this,SIGNAL(segmentationChanged()));
}

void CDTSegmentationLayer::updateTreeModel(CDTProjectTreeItem *parent)
{
    CDTProjectTreeItem *segment =new CDTProjectTreeItem(
                CDTProjectTreeItem::SEGMENTION,CDTProjectTreeItem::GROUP,m_name,this);
    CDTProjectTreeItem *paramShp =new CDTProjectTreeItem(
                CDTProjectTreeItem::PARAM,CDTProjectTreeItem::VECTOR,tr("Shapefile path"),this);
    CDTProjectTreeItem *valueShp =new CDTProjectTreeItem(
                CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,m_shapefilePath,this);
    CDTProjectTreeItem *paramMk =new CDTProjectTreeItem(
                CDTProjectTreeItem::PARAM,CDTProjectTreeItem::EMPTY,tr("Markfile path"),this);
    CDTProjectTreeItem *valueMk =new CDTProjectTreeItem(
                CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,m_markfilePath,this);
    CDTProjectTreeItem *methodroot =new CDTProjectTreeItem(
                CDTProjectTreeItem::METHOD_PARAMS,CDTProjectTreeItem::EMPTY,tr("Method"),this);
    CDTProjectTreeItem *methodrootvalue =new CDTProjectTreeItem(
                CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,m_method,this);
    CDTProjectTreeItem *classificationsroot =new CDTProjectTreeItem(
                CDTProjectTreeItem::CLASSIFICATION_ROOT,CDTProjectTreeItem::EMPTY,tr("Classifications"),this);

    for(int i=0;i<m_params.size();++i)
    {
        QList<QString> keys =m_params.keys();
        CDTProjectTreeItem *methodparam =new CDTProjectTreeItem(
                    CDTProjectTreeItem::PARAM,CDTProjectTreeItem::EMPTY,keys[i],this);
        CDTProjectTreeItem *methodvalue =new CDTProjectTreeItem(
                    CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,m_params[keys[i]].toString(),this);
        methodroot->setChild(i,0,methodparam);
        methodroot->setChild(i,1,methodvalue);
    }

    segment->setChild(0,0,paramShp);
    segment->setChild(0,1,valueShp);
    segment->setChild(1,0,paramMk);
    segment->setChild(1,1,valueMk);
    segment->setChild(2,0,methodroot);
    segment->setChild(2,1,methodrootvalue);
    segment->setChild(3,classificationsroot);

    parent->appendRow(segment);

    for(int i=0;i<classifications.size();++i)
    {
        classifications[i]->updateTreeModel(classificationsroot);
    }
}

void CDTSegmentationLayer::onContextMenuRequest(QWidget *parent)
{
    actionRemoveSegmentation->setIcon(QIcon(":/Icon/remove.png"));
    actionRemoveAllClassifications->setIcon(QIcon(":/Icon/remove.png"));
    addClassifications->setIcon(QIcon(":/Icon/add.png"));
    actionRename->setIcon(QIcon(":/Icon/rename.png"));
    QMenu *menu =new QMenu(parent);
    menu->addAction(addClassifications);
    menu->addAction(actionRemoveSegmentation);
    menu->addAction(actionRemoveAllClassifications);
    menu->addSeparator();
    menu->addAction(actionRename);
    menu->exec(QCursor::pos());
}

void CDTSegmentationLayer::onActionRename()
{
    bool ok;
    QString text = QInputDialog::getText(
                NULL, tr("Input Segmentation Name"),
                tr("Segmentation name:"), QLineEdit::Normal,
                m_name, &ok);
    if (ok && !text.isEmpty())
        setName(text);
}

void CDTSegmentationLayer::addClassification()
{
    QMap<QString,QVariant> param;
    param["k"] ="new k";

    CDTClassification *classification =new CDTClassification(this);
    classification->setName("new classification");
    classification->setShapefilePath("new shapefilepath");
    classification->setMethodParams("new knn",param);

    addClassification(classification);
}

void CDTSegmentationLayer::remove()
{
    emit removeSegmentation(this);
}

void CDTSegmentationLayer::removeClassification(CDTClassification* clf)
{
    for(int i =0;i <classifications.size();++i)
    {
        if(clf->name() == classifications[i]->name())
        {
            classifications.remove(i);
            emit segmentationChanged();
            connect(clf,SIGNAL(methodParamsChanged()),this,SIGNAL(segmentationChanged()));

        }
    }
}

void CDTSegmentationLayer::removeAllClassifications()
{
    classifications.clear();
    emit segmentationChanged();
}

void CDTSegmentationLayer::onVectorLayerProperty()
{

}

QString CDTSegmentationLayer::name() const
{
    return m_name;
}

QString CDTSegmentationLayer::shapefilePath() const
{
    return m_shapefilePath;
}

QString CDTSegmentationLayer::markfilePath() const
{
    return m_markfilePath;
}

QString CDTSegmentationLayer::method() const
{
    return m_method;
}

CDTDatabaseConnInfo CDTSegmentationLayer::databaseURL() const
{
    return m_dbUrl;
}

QString CDTSegmentationLayer::imagePath() const
{
    return m_imagePath;
}

void CDTSegmentationLayer::setName(const QString &name)
{
    m_name = name;
    keyItem->setText(m_name);
    emit nameChanged();
}

void CDTSegmentationLayer::setShapefilePath(const QString &shpPath)
{
    if (m_shapefilePath == shpPath)
        return;
    m_shapefilePath = shpPath;
    shapefileItem->setText(m_shapefilePath);
    if (mapCanvasLayer)
        delete mapCanvasLayer;
    mapCanvasLayer = new QgsVectorLayer(/*QFileInfo(shpPath).absolutePath()*/shpPath,QFileInfo(shpPath).completeBaseName(),"ogr");
    if (!mapCanvasLayer->isValid())
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Open shapefile ")+shpPath+tr(" failed!"));
        delete mapCanvasLayer;
        return;
    }

    QgsVectorLayer*p = (QgsVectorLayer*)mapCanvasLayer;
    QgsSimpleFillSymbolLayerV2* symbolLayer = new QgsSimpleFillSymbolLayerV2();
    symbolLayer->setColor(QColor(0,0,0,0));
    symbolLayer->setBorderColor(QColor(qrand()%255,qrand()%255,qrand()%255));
    QgsFillSymbolV2 *fillSymbol = new QgsFillSymbolV2(QgsSymbolLayerV2List()<<symbolLayer);
    QgsSingleSymbolRendererV2* singleSymbolRenderer = new QgsSingleSymbolRendererV2(fillSymbol);
    p->setRendererV2(singleSymbolRenderer);
    QgsMapLayerRegistry::instance()->addMapLayer(mapCanvasLayer);
    keyItem->setMapLayer(mapCanvasLayer);


    emit appendLayers(QList<QgsMapLayer*>()<<mapCanvasLayer);
    emit shapefilePathChanged();
}

void CDTSegmentationLayer::setMarkfilePath(const QString &mkPath)
{
    m_markfilePath = mkPath;
    markfileItem->setText(m_markfilePath);
    emit markfilePathChanged();
}

void CDTSegmentationLayer::setMethodParams(const QString &methodName, const QMap<QString, QVariant> &params)
{
    m_method = methodName;
    paramRootItem->removeRows(0,paramRootItem->rowCount());
    paramRootValueItem->setText(m_method);
    m_params = params;
    QStringList keys = m_params.keys();
    foreach (QString key, keys) {
        QVariant value = m_params.value(key);
        paramRootItem->appendRow(
                    QList<QStandardItem*>()
                    <<new CDTProjectTreeItem(CDTProjectTreeItem::PARAM,CDTProjectTreeItem::EMPTY,key,this)
                    <<new CDTProjectTreeItem(CDTProjectTreeItem::VALUE,CDTProjectTreeItem::EMPTY,value.toString(),this)
                    );
    }
}

void CDTSegmentationLayer::setDatabaseURL(CDTDatabaseConnInfo url)
{
    m_dbUrl = url;
    emit segmentationChanged();
}

QDataStream &operator<<(QDataStream &out, const CDTSegmentationLayer &segmentation)
{    
    out<<segmentation.m_name<<segmentation.m_shapefilePath<<segmentation.m_markfilePath
      <<segmentation.m_method<<segmentation.m_params<<segmentation.m_dbUrl;

    out<<segmentation.classifications.size();
    for (int i=0;i<segmentation.classifications.size();++i)
        out<<*(segmentation.classifications[i]);

    return out;
}


QDataStream &operator>>(QDataStream &in,CDTSegmentationLayer &segmentation)
{
    QString temp;
    in>>temp;

    segmentation.setName(temp);
    in>>temp;

    segmentation.setShapefilePath(temp);
    in>>temp;

    segmentation.setMarkfilePath(temp);
    in>>temp;

    QMap<QString,QVariant> paramsTemp;
    in>>paramsTemp;
    segmentation.setMethodParams(temp,paramsTemp);
    in>>segmentation.m_dbUrl;



    int count;
    in>>count;
    for (int i=0;i<count;++i)
    {
        CDTClassification* classification = new CDTClassification(&segmentation);
        in>>*classification;
        segmentation.classifications.push_back(classification);
    }
    return in;
}
