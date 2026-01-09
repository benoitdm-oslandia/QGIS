/***************************************************************************
     testqgs3dmaptoolcreateprimitive.cpp
     --------------------------------
    begin                : December 2025
    copyright            : (C) 2025 by Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgisapp.h"
#include "qgs3d.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmapcanvaswidget.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmaptoolcreateprimitive.h"
#include "qgs3drendercontext.h"
#include "qgs3dutils.h"
#include "qgsdemterraingenerator.h"
#include "qgsdemterrainsettings.h"
#include "qgsflatterraingenerator.h"
#include "qgsflatterrainsettings.h"
#include "qgsgeometry.h"
#include "qgsoffscreen3dengine.h"
#include "qgspointlightsettings.h"
#include "qgspolygon3dsymbol.h"
#include "qgsrasterlayer.h"
#include "qgstest.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayer3drenderer.h"
#include "qgswindow3dengine.h"
#include "testqgs3dmaptoolutils.h"

class Qgs3DMapToolCreatePrimitiveForTest : public Qgs3DMapToolCreatePrimitive
{
  public:
    Qgs3DMapToolCreatePrimitiveForTest( Qgs3DMapCanvas *canvas, PrimitiveType type )
      : Qgs3DMapToolCreatePrimitive( canvas, type )
    {
      mShowAttributeValuesDlg = false;
      mShowPrimitiveDialog = false;
    }

    double constraintMapPointForTest( QgsPoint &pointMap, const Qt::KeyboardModifiers &stateKey )
    {
      return constraintMapPoint( pointMap, stateKey );
    }

    QgsPoint screenToMapForTest( const QPoint &screenPos ) const
    {
      return screenToMap( screenPos );
    }
};

/**
 * \ingroup UnitTests
 * This is a unit test for the vertex tool
 */
class TestQgs3DMapToolCreatePrimitive : public QgsTest
{
    Q_OBJECT
  public:
    TestQgs3DMapToolCreatePrimitive();

  private slots:
    void initTestCase();    // will be called before the first testfunction is executed.
    void cleanupTestCase(); // will be called after the last testfunction was executed.

    void testCreateCube();
    void testCreateCubeConstraint();

  private:
    bool compareGeom( const QgsGeometry &geom, const QString &wkt, double tolerance );

    QgisApp *mQgisApp = nullptr;
    Qgs3DMapCanvas *mCanvas = nullptr;
    QgsVectorLayer *mLayerBase = nullptr;
    QgsProject *mProject = nullptr;
    QgsRasterLayer *mLayerDtm = nullptr;
};

TestQgs3DMapToolCreatePrimitive::TestQgs3DMapToolCreatePrimitive()
  : QgsTest( u"3D MapTool Create Primitive"_s, u"3d"_s )
{}


bool TestQgs3DMapToolCreatePrimitive::compareGeom( const QgsGeometry &geom, const QString &wkt, double tolerance )
{
  QgsGeometry geomB = QgsGeometry::fromWkt( wkt );
  bool out = geom.constGet()->fuzzyEqual( *geomB.constGet(), tolerance );
  if ( !out )
  {
    qDebug() << "Failure with actual:" << geom.asWkt( 2 );
    qDebug() << "           expected:" << geomB.asWkt( 2 );
  }
  return out;
}

//runs before all tests
void TestQgs3DMapToolCreatePrimitive::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  // Set up the QSettings environment
  QCoreApplication::setOrganizationName( u"QGIS"_s );
  QCoreApplication::setOrganizationDomain( u"qgis.org"_s );
  QCoreApplication::setApplicationName( u"QGIS-TEST"_s );

  mQgisApp = new QgisApp();

  mProject = new QgsProject();
  mProject->setCrs( QgsCoordinateReferenceSystem::fromOgcWmsCrs( "EPSG:27700" ) );

  mLayerDtm = new QgsRasterLayer( testDataPath( "/3d/dtm.tif" ), "dtm", "gdal" );
  QVERIFY( mLayerDtm->isValid() );
  mProject->addMapLayer( mLayerDtm );
  const QgsRectangle fullExtent = mLayerDtm->extent();

  // make testing layers
  mLayerBase = QgisApp::instance()->addVectorLayer( u"PolyhedralSurface?crs=EPSG:27700"_s, u"baselayer"_s, u"memory"_s );
  QVERIFY( mLayerBase->id() != "" );
  QVERIFY( mLayerBase->isValid() );
  mProject->addMapLayer( mLayerBase );

  mLayerBase->startEditing();
  const QString wkt1 = u"PolyhedralSurface Z (((0 0 0, 0 1 5, 1 1 10, 1 0 5, 0 0 0)))"_s;
  QgsFeature f1;
  QgsGeometry g1 = QgsGeometry::fromWkt( wkt1 );
  QVERIFY( !g1.isEmpty() );
  g1.translate( fullExtent.center().x(), fullExtent.center().y(), 500 );
  f1.setGeometry( g1 );
  qDebug() << "FEATURE GEOM1:" << f1.geometry().asWkt( 1 );

  const QString wkt2 = u"PolyhedralSurface Z (((2 0 0, 2 5 5, 3 5 10, 3 0 10, 2 5 5, 3 5 5, 2 0 0)))"_s;
  QgsFeature f2;
  QgsGeometry g2 = QgsGeometry::fromWkt( wkt2 );
  QVERIFY( !g2.isEmpty() );
  g2.translate( fullExtent.center().x(), fullExtent.center().y(), 500 );
  f2.setGeometry( g2 );

  QgsFeatureList flist;
  flist << f1 << f2;
  mLayerBase->dataProvider()->addFeatures( flist );

  QCOMPARE( mLayerBase->featureCount(), 2L );

  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::lightGray );
  QgsPolygon3DSymbol *symbol3d = new QgsPolygon3DSymbol;
  symbol3d->setMaterialSettings( materialSettings.clone() );
  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbol3d );
  mLayerBase->setRenderer3D( renderer3d );

  Qgs3DMapSettings *mapSettings = new Qgs3DMapSettings;
  mapSettings->setCrs( mProject->crs() );
  mapSettings->setExtent( fullExtent );
  mapSettings->setLayers( { mLayerBase } );

  QgsPointLightSettings defaultLight;
  defaultLight.setIntensity( 0.5 );
  defaultLight.setPosition( mapSettings->origin() + QgsVector3D( 0, 0, 1000 ) );
  mapSettings->setLightSources( { defaultLight.clone() } );

  QgsDemTerrainSettings *demTerrainSettings = new QgsDemTerrainSettings;
  demTerrainSettings->setLayer( mLayerDtm );
  demTerrainSettings->setVerticalScale( 1 );
  mapSettings->setTerrainSettings( demTerrainSettings );

  std::unique_ptr<QgsTerrainGenerator> generator = demTerrainSettings->createTerrainGenerator( Qgs3DRenderContext::fromMapSettings( mapSettings ) );
  QVERIFY( dynamic_cast<QgsDemTerrainGenerator *>( generator.get() )->isValid() );
  QCOMPARE( dynamic_cast<QgsDemTerrainGenerator *>( generator.get() )->layer(), mLayerDtm );

  mCanvas = new Qgs3DMapCanvas();
  mCanvas->resize( 640, 480 );
  mCanvas->show(); // to make the canvas resize
  mCanvas->hide();
  mCanvas->setMapSettings( mapSettings );

  Qgs3DMapScene *scene = mCanvas->scene();
  QgsWindow3DEngine *engine = mCanvas->engine();

  mCanvas->cameraController()->setLookingAtPoint( QVector3D( 0, 0, 0 ), 600, 0, 0 );
  Qgs3DUtils::captureSceneImage( *dynamic_cast<QgsAbstract3DEngine *>( engine ), scene );
}

//runs after all tests
void TestQgs3DMapToolCreatePrimitive::cleanupTestCase()
{
  mCanvas->deleteLater();
  mLayerDtm->deleteLater();
  mProject->deleteLater();
  QgsApplication::exitQgis();
  mQgisApp->deleteLater();
}

void TestQgs3DMapToolCreatePrimitive::testCreateCube()
{
  QVERIFY( QgisApp::instance()->setActiveLayer( mLayerBase ) );

  // create the tool
  Qgs3DMapToolCreatePrimitiveForTest mMapTool( mCanvas, Qgs3DMapToolCreatePrimitive::Box );
  mCanvas->setMapTool( &mMapTool );
  TestQgs3DMapToolUtils utils( &mMapTool, mCanvas->size() );
  QgsPoint ptMap;
  QPoint ptScreen;

  ptScreen = QPoint( 1, 1 ); // start point
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321625, 130106 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  ptScreen = QPoint( 2, 8 ); // x (add less x than y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321626, 130100 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::KeyboardModifiers() ), 341.6, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321626, 130100 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  ptScreen = QPoint( 12, 12 ); // y (add more x than y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321634, 130097 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::KeyboardModifiers() ), 337.1, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321634, 130097 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  ptScreen = QPoint( 16, 16 ); // z (add same x and y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321638, 130093 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::KeyboardModifiers() ), 341.8, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321638, 130093 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  utils.keyClick( Qt::Key_Enter );

  QCOMPARE( mLayerBase->featureCount(), 3L );

  QgsFeatureIterator ite = mLayerBase->getFeatures();
  QgsFeature f;
  while ( ite.nextFeature( f ) )
  {
    if ( f.id() < 0 )
    {
      const QString wkt = "PolyhedralSurface Z ("
                          "((321897.7 129894.2 76.9, 321897.7 129904.0 76.9, 321904.1 129904.0 76.9, 321904.1 129894.2 76.9, 321897.7 129894.2 76.9)),"
                          "((321897.7 129894.2 82.0, 321904.1 129894.2 82.0, 321904.1 129904.0 82.0, 321897.7 129904.0 82.0, 321897.7 129894.2 82.0)),"
                          "((321897.7 129894.2 76.9, 321904.1 129894.2 76.9, 321904.1 129894.2 82.0, 321897.7 129894.2 82.0, 321897.7 129894.2 76.9)),"
                          "((321897.7 129904.0 76.9, 321897.7 129904.0 82.0, 321904.1 129904.0 82.0, 321904.1 129904.0 76.9, 321897.7 129904.0 76.9)),"
                          "((321904.1 129894.2 76.9, 321904.1 129904.0 76.9, 321904.1 129904.0 82.0, 321904.1 129894.2 82.0, 321904.1 129894.2 76.9)),"
                          "((321897.7 129894.2 76.9, 321897.7 129894.2 82.0, 321897.7 129904.0 82.0, 321897.7 129904.0 76.9, 321897.7 129894.2 76.9)))";
      QVERIFY( compareGeom( f.geometry(), wkt, 0.05 ) );
      break;
    }
  }

  mLayerBase->undoStack()->undo();
  mCanvas->setMapTool( nullptr );
}

void TestQgs3DMapToolCreatePrimitive::testCreateCubeConstraint()
{
  QVERIFY( QgisApp::instance()->setActiveLayer( mLayerBase ) );

  // create the tool
  Qgs3DMapToolCreatePrimitiveForTest mMapTool( mCanvas, Qgs3DMapToolCreatePrimitive::Box );
  mCanvas->setMapTool( &mMapTool );
  TestQgs3DMapToolUtils utils( &mMapTool, mCanvas->size() );
  QgsPoint ptMap;
  QPoint ptScreen;

  ptScreen = QPoint( 1, 1 ); // start point
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321625, 130106 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  ptScreen = QPoint( 2, 8 ); // x (add less x than y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321626, 130100 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::ControlModifier ), 275.2, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321626, 129899 ), 1.0 );
  utils.mouseMove( ptScreen, Qt::ControlModifier );
  utils.mouseClick( ptScreen, Qt::LeftButton, Qt::ControlModifier );

  ptScreen = QPoint( 12, 12 ); // y (add more x than y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321634, 130097 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::ControlModifier ), 197.7, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321902, 130097 ), 1.0 );
  utils.mouseMove( ptScreen, Qt::ControlModifier );
  utils.mouseClick( ptScreen, Qt::LeftButton, Qt::ControlModifier );

  ptScreen = QPoint( 16, 16 ); // z (add same x and y)
  ptMap = mMapTool.screenToMapForTest( ptScreen );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321638, 130093 ), 1.0 );
  QGSCOMPARENEAR( mMapTool.constraintMapPointForTest( ptMap, Qt::ControlModifier ), 22.4, 0.5 );
  QGSCOMPARENEARPOINT( ptMap, QgsPoint( 321902, 129889 ), 1.0 );
  utils.mouseMove( ptScreen );
  utils.mouseClick( ptScreen, Qt::LeftButton );

  utils.keyClick( Qt::Key_Enter );

  QCOMPARE( mLayerBase->featureCount(), 3L );

  QgsFeatureIterator ite = mLayerBase->getFeatures();
  QgsFeature f;
  while ( ite.nextFeature( f ) )
  {
    if ( f.id() < 0 )
    {
      const QString wkt = "PolyhedralSurface Z ("
                          "((321900.4 129894.1 76.9, 321900.4 129904.1 76.9, 321901.3 129904.1 76.9, 321901.3 129894.1 76.9, 321900.4 129894.1 76.9)),"
                          "((321900.4 129894.1 90.2, 321901.3 129894.1 90.2, 321901.3 129904.1 90.2, 321900.4 129904.1 90.2, 321900.4 129894.1 90.2)),"
                          "((321900.4 129894.1 76.9, 321901.3 129894.1 76.9, 321901.3 129894.1 90.2, 321900.4 129894.1 90.2, 321900.4 129894.1 76.9)),"
                          "((321900.4 129904.1 76.9, 321900.4 129904.1 90.2, 321901.3 129904.1 90.2, 321901.3 129904.1 76.9, 321900.4 129904.1 76.9)),"
                          "((321901.3 129894.1 76.9, 321901.3 129904.1 76.9, 321901.3 129904.1 90.2, 321901.3 129894.1 90.2, 321901.3 129894.1 76.9)),"
                          "((321900.4 129894.1 76.9, 321900.4 129894.1 90.2, 321900.4 129904.1 90.2, 321900.4 129904.1 76.9, 321900.4 129894.1 76.9)))";
      QVERIFY( compareGeom( f.geometry(), wkt, 0.05 ) );
      break;
    }
  }

  mLayerBase->undoStack()->undo();
  mCanvas->setMapTool( nullptr );
}

QGSTEST_MAIN( TestQgs3DMapToolCreatePrimitive )
#include "testqgs3dmaptoolcreateprimitive.moc"
