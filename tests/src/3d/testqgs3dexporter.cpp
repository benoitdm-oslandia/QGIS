/***************************************************************************
  testqgs3dexporter.cpp
  --------------------------------------
  Date                 : September 2025
  Copyright            : (C) 2025 by Jean Felder
  Email                : jean dot felder at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3d.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3drendercontext.h"
#include "qgs3dsceneexporter.h"
#include "qgs3dsymbolregistry.h"
#include "qgs3dutils.h"
#include "qgscameracontroller.h"
#include "qgsdemterraingenerator.h"
#include "qgsdemterrainsettings.h"
#include "qgsfeature3dhandler_p.h"
#include "qgsflatterraingenerator.h"
#include "qgsflatterrainsettings.h"
#include "qgsline3dsymbol.h"
#include "qgsoffscreen3dengine.h"
#include "qgspoint3dsymbol.h"
#include "qgspointlightsettings.h"
#include "qgspolygon3dsymbol.h"
#include "qgsrasterlayer.h"
#include "qgstest.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayer3drenderer.h"

#include <QString>

using namespace Qt::StringLiterals;

class TestQgs3DExporter : public QgsTest
{
    Q_OBJECT

  public:
    TestQgs3DExporter()
      : QgsTest( u"3D Exporter Tests"_s, u"3d"_s )
    {}

  private slots:
    void initTestCase();    // will be called before the first testfunction is executed.
    void cleanupTestCase(); // will be called after the last testfunction was executed.
    void test3DSceneExporter();
    void test3DSceneExporterBig();
    void test3DSceneExporterFlatTerrain();
    void test3DSceneExporterInstanced();
    void testChunkedEntityElevationDtm();
    void test3DSceneExporterDEM();

  private:
    void do3DSceneExport(
      const QString &testName,
      int zoomLevelsCount,
      int expectedObjectCount,
      int expectedFeatureCount,
      int maxFaceCount,
      Qgs3DMapScene *scene,
      QgsVectorLayer *layerPoly,
      QgsOffscreen3DEngine *engine,
      QgsTerrainEntity *terrainEntity = nullptr,
      Qgis::AltitudeClamping altitudeClamping = Qgis::AltitudeClamping::Absolute,
      Qgis::AltitudeBinding altitudeBinding = Qgis::AltitudeBinding::Centroid
    );

    /**
     * Checks projet demgenerator has got fine dtm elevation data and checks feature Z coordinates
     * \param fullExtent
     * \param crs
     * \param dataPath data layer path
     * \param dtmPath dtm layer path
     * \param dtmHiResPath dtm layer path will be used with no cache
     * \param featExtent extent to find the feature
     * \param expectedZ feature Z coordinate values
     * \param expectedCacheSize
     */
    void doCheckElevation(
      const QgsRectangle &fullExtent,
      const QgsCoordinateReferenceSystem &crs,
      const QString &dataPath,
      const QString &dtmPath,
      const QString &dtmHiResPath,
      const QgsRectangle &featExtent,
      const QVector<float> &expectedZ,
      float expectedCacheSize
    );

    void doCheckFeatureZ( QgsFeature f, QVector<float> expectedZ, Qgs3DMapSettings *map, QgsAbstract3DSymbol *symbolTerrain, QgsVectorLayer *layerData );

    QgsVectorLayer *mLayerBuildings = nullptr;
};

// runs before all tests
void TestQgs3DExporter::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  mLayerBuildings = new QgsVectorLayer( testDataPath( "/3d/buildings.shp" ), "buildings", "ogr" );
  QVERIFY( mLayerBuildings->isValid() );

  // best to keep buildings without 2D renderer so it is not painted on the terrain
  // so we do not get some possible artifacts
  mLayerBuildings->setRenderer( nullptr );

  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::lightGray );
  QgsPolygon3DSymbol *symbol3d = new QgsPolygon3DSymbol;
  symbol3d->setMaterialSettings( materialSettings.clone() );
  symbol3d->setExtrusionHeight( 10.f );
  symbol3d->setAltitudeClamping( Qgis::AltitudeClamping::Relative );
  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbol3d );
  mLayerBuildings->setRenderer3D( renderer3d );
}

//runs after all tests
void TestQgs3DExporter::cleanupTestCase()
{
  QgsApplication::exitQgis();
}

void TestQgs3DExporter::do3DSceneExport(
  const QString &testName,
  int zoomLevelsCount,
  int expectedObjectCount,
  int expectedFeatureCount,
  int maxFaceCount,
  Qgs3DMapScene *scene,
  QgsVectorLayer *layerPoly,
  QgsOffscreen3DEngine *engine,
  QgsTerrainEntity *terrainEntity,
  Qgis::AltitudeClamping altitudeClamping,
  Qgis::AltitudeBinding altitudeBinding
)
{
  qDebug() << "Will check 3D scene export for" << testName << ", zoom:" << zoomLevelsCount;

  // 3d renderer must be replaced to have the tiling updated
  QVERIFY( layerPoly->renderer3D() != nullptr );
  QgsVectorLayer3DRenderer *oldRenderer3d = dynamic_cast<QgsVectorLayer3DRenderer *>( layerPoly->renderer3D() );
  QVERIFY( oldRenderer3d != nullptr );
  QgsPolygon3DSymbol *symbol3d = dynamic_cast<QgsPolygon3DSymbol *>( oldRenderer3d->symbol()->clone() );
  symbol3d->setAltitudeClamping( altitudeClamping );
  symbol3d->setAltitudeBinding( altitudeBinding );
  QgsVectorLayer3DRenderer *newRenderer3d = new QgsVectorLayer3DRenderer( symbol3d );

  QgsVectorLayer3DTilingSettings tilingSettings;
  tilingSettings.setZoomLevelsCount( zoomLevelsCount );
  tilingSettings.setShowBoundingBoxes( true );
  newRenderer3d->setTilingSettings( tilingSettings );
  layerPoly->setRenderer3D( newRenderer3d );

  // Calling captureSceneImage to wait for Qgs3DMapScene::Ready
  Qgs3DUtils::captureSceneImage( *engine, scene );
  QCOMPARE( scene->sceneState(), Qgs3DMapScene::Ready );

  Qgs3DSceneExporter exporter;
  exporter.setTerrainResolution( 128 );
  exporter.setSmoothEdges( false );
  exporter.setExportNormals( true );
  exporter.setExportTextures( false );
  exporter.setTerrainTextureResolution( 512 );
  exporter.setScale( 1.0 );

  QVERIFY( exporter.parseVectorLayerEntity( scene->layerEntity( layerPoly ), layerPoly ) );
  if ( terrainEntity )
    exporter.parseTerrain( terrainEntity, "DEM_Tile" );

  QString objFileName = u"%1-%2"_s.arg( testName ).arg( zoomLevelsCount );
  const bool saved = exporter.save( objFileName, QDir::tempPath(), 3 );
  QVERIFY( saved );

  size_t sum = 0;
  for ( Qgs3DExportObject *o : std::as_const( exporter.mObjects ) )
  {
    if ( !terrainEntity ) // not compatible with terrain entity
      QVERIFY( o->indexes().size() <= o->vertexPosition().size() );
    sum += o->indexes().size();
  }

  QCOMPARE( sum, maxFaceCount );
  QCOMPARE( exporter.mExportedFeatureIds.size(), expectedFeatureCount );
  QCOMPARE( exporter.mObjects.size(), expectedObjectCount );

  QFile file( QString( "%1/%2.obj" ).arg( QDir::tempPath(), objFileName ) );
  QVERIFY( file.open( QIODevice::ReadOnly | QIODevice::Text ) );
  QTextStream fileStream( &file );

  // check the generated obj file
  QGSCOMPARELONGSTR( testName.toStdString().c_str(), u"%1.obj"_s.arg( testName ), fileStream.readAll().toUtf8() );
}

void TestQgs3DExporter::test3DSceneExporter()
{
  // =============================================
  // =========== creating Qgs3DMapSettings
  QgsVectorLayer *layerPoly = new QgsVectorLayer( testDataPath( u"/3d/polygons.gpkg.gz"_s ), u"polygons"_s, u"ogr"_s );
  QVERIFY( layerPoly->isValid() );

  const QgsRectangle fullExtent = layerPoly->extent();

  // =========== create polygon 3D renderer
  QgsPolygon3DSymbol *symbol3d = new QgsPolygon3DSymbol();
  symbol3d->setExtrusionHeight( 10.f );
  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::lightGray );
  symbol3d->setMaterialSettings( materialSettings.clone() );

  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbol3d );
  layerPoly->setRenderer3D( renderer3d );

  QgsProject project;
  project.setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 3857 ) );
  project.addMapLayer( layerPoly );

  // =========== create scene 3D settings
  Qgs3DMapSettings mapSettings;
  mapSettings.setCrs( project.crs() );
  mapSettings.setExtent( fullExtent );
  mapSettings.setLayers( { layerPoly } );

  mapSettings.setTransformContext( project.transformContext() );
  mapSettings.setPathResolver( project.pathResolver() );
  mapSettings.setMapThemeCollection( project.mapThemeCollection() );
  mapSettings.setOutputDpi( 92 );

  // =========== creating Qgs3DMapScene
  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( mapSettings, &engine );

  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 7000, 20.0, -10.0 );
  engine.setRootEntity( scene );

  const int nbFaces = 165;
  const int nbFeat = 3;

  // =========== check with 1 big tile ==> 1 exported object
  do3DSceneExport( u"scene_export"_s, 1, 1, nbFeat, nbFaces, scene, layerPoly, &engine );
  // =========== check with 4 tiles ==> 1 exported objects
  do3DSceneExport( u"scene_export"_s, 2, 1, nbFeat, nbFaces, scene, layerPoly, &engine );
  // =========== check with 9 tiles ==> 3 exported objects
  do3DSceneExport( u"scene_export"_s, 3, 3, nbFeat, nbFaces, scene, layerPoly, &engine );
  // =========== check with 16 tiles ==> 3 exported objects
  do3DSceneExport( u"scene_export"_s, 4, 3, nbFeat, nbFaces, scene, layerPoly, &engine );
  // =========== check with 25 tiles ==> 3 exported objects
  do3DSceneExport( u"scene_export"_s, 5, 3, nbFeat, nbFaces, scene, layerPoly, &engine );

  delete scene;
  mapSettings.setLayers( {} );
}

void TestQgs3DExporter::test3DSceneExporterDEM()
{
  // =============================================
  // =========== creating Qgs3DMapSettings
  QgsRasterLayer *layerDtm = new QgsRasterLayer( testDataPath( "/3d/dtm.tif" ), "dtm", "gdal" );
  QVERIFY( layerDtm->isValid() );

  QgsVectorLayer *layerPoly = new QgsVectorLayer( testDataPath( "/3d/polygons.gpkg.gz" ), "polygons", "ogr" );
  QVERIFY( layerPoly->isValid() );

  const QgsRectangle fullExtent = layerPoly->extent();

  // =========== create polygon 3D renderer
  QgsPolygon3DSymbol *symbol3d = new QgsPolygon3DSymbol();
  symbol3d->setExtrusionHeight( 10.f );
  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::lightGray );
  symbol3d->setMaterialSettings( materialSettings.clone() );

  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbol3d );
  layerPoly->setRenderer3D( renderer3d );

  QgsProject project;
  project.setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 3857 ) );
  project.addMapLayer( layerDtm );
  project.addMapLayer( layerPoly );

  // =========== create scene 3D settings
  Qgs3DMapSettings mapSettings;
  mapSettings.setCrs( project.crs() );
  mapSettings.setExtent( fullExtent );
  mapSettings.setLayers( { layerDtm, layerPoly } );

  mapSettings.setTransformContext( project.transformContext() );
  mapSettings.setPathResolver( project.pathResolver() );
  mapSettings.setMapThemeCollection( project.mapThemeCollection() );
  mapSettings.setOutputDpi( 92 );

  QgsDemTerrainSettings *demTerrainSettings = new QgsDemTerrainSettings;
  demTerrainSettings->setLayer( layerDtm );
  demTerrainSettings->setVerticalScale( 3 );
  demTerrainSettings->setElevationOffset( -1000.0 );
  mapSettings.setTerrainSettings( demTerrainSettings );

  // =========== creating Qgs3DMapScene
  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( mapSettings, &engine );

  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 7000, 20.0, -10.0 );
  engine.setRootEntity( scene );

  const int nbFaces = 165;
  const int nbFeat = 3;

  // =========== check with 4 tiles ==> 1 exported objects
  do3DSceneExport( "scene_export_dem", 2, 1, nbFeat, nbFaces, scene, layerPoly, &engine, nullptr, Qgis::AltitudeClamping::Terrain, Qgis::AltitudeBinding::Centroid );
  // =========== check with 25 tiles ==> 3 exported objects + DEM
  do3DSceneExport( "scene_export_dem", 5, 3, nbFeat, nbFaces, scene, layerPoly, &engine, nullptr, Qgis::AltitudeClamping::Terrain, Qgis::AltitudeBinding::Centroid );

  delete scene;
  mapSettings.setLayers( {} );
}

void TestQgs3DExporter::test3DSceneExporterBig()
{
  // =============================================
  // =========== creating layers and project
  QgsRasterLayer *layerDtm = new QgsRasterLayer( testDataPath( u"/3d/dtm.tif"_s ), u"dtm"_s, u"gdal"_s );
  QVERIFY( layerDtm->isValid() );

  const QgsRectangle fullExtent = layerDtm->extent();

  QgsProject project;
  project.setCrs( layerDtm->crs() );
  project.addMapLayer( layerDtm );
  project.addMapLayer( mLayerBuildings );

  // =============================================
  // =========== creating Qgs3DMapSettings
  Qgs3DMapSettings mapSettings;
  mapSettings.setCrs( project.crs() );
  mapSettings.setExtent( fullExtent );
  mapSettings.setLayers( { layerDtm, mLayerBuildings } );

  mapSettings.setTransformContext( project.transformContext() );
  mapSettings.setPathResolver( project.pathResolver() );
  mapSettings.setMapThemeCollection( project.mapThemeCollection() );

  QgsDemTerrainSettings *demTerrainSettings = new QgsDemTerrainSettings;
  demTerrainSettings->setLayer( layerDtm );
  demTerrainSettings->setVerticalScale( 3 );
  mapSettings.setTerrainSettings( demTerrainSettings );

  QgsPointLightSettings defaultPointLight;
  defaultPointLight.setPosition( mapSettings.origin() + QgsVector3D( 0, 400, 0 ) );
  defaultPointLight.setConstantAttenuation( 0 );
  mapSettings.setLightSources( { defaultPointLight.clone() } );
  mapSettings.setOutputDpi( 92 );

  // =========== creating Qgs3DMapScene
  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( mapSettings, &engine );
  engine.setRootEntity( scene );

  // =========== set camera position
  scene->cameraController()->setLookingAtPoint( QVector3D( 0, 0, 0 ), 1500, 40.0, -10.0 );

  const int nbFaces = 19869;
  const int nbFeat = 401;

  // =========== check with 1 big tile ==> 1 exported object
  do3DSceneExport( u"big_scene_export"_s, 1, 1, nbFeat, nbFaces, scene, mLayerBuildings, &engine );
  // =========== check with 4 tiles ==> 4 exported objects
  do3DSceneExport( u"big_scene_export"_s, 2, 4, nbFeat, nbFaces, scene, mLayerBuildings, &engine );
  // =========== check with 9 tiles ==> 14 exported objects
  do3DSceneExport( u"big_scene_export"_s, 3, 14, nbFeat, nbFaces, scene, mLayerBuildings, &engine );
  // =========== check with 16 tiles ==> 32 exported objects
  do3DSceneExport( u"big_scene_export"_s, 4, 32, nbFeat, nbFaces, scene, mLayerBuildings, &engine );
  // =========== check with 25 tiles ==> 70 exported objects
  do3DSceneExport( u"big_scene_export"_s, 5, 70, nbFeat, nbFaces, scene, mLayerBuildings, &engine );

  // =========== check with 25 tiles + terrain ==> 70+1 exported objects
  do3DSceneExport( u"terrain_scene_export"_s, 5, 71, nbFeat, 119715, scene, mLayerBuildings, &engine, scene->terrainEntity() );

  delete scene;
  mapSettings.setLayers( {} );
}

void TestQgs3DExporter::test3DSceneExporterFlatTerrain()
{
  QgsRasterLayer *layerRgb = new QgsRasterLayer( testDataPath( u"/3d/rgb.tif"_s ), u"rgb"_s, u"gdal"_s );
  QVERIFY( layerRgb->isValid() );

  const QgsRectangle fullExtent = layerRgb->extent();

  QgsProject project;
  project.setCrs( layerRgb->crs() );
  project.addMapLayer( layerRgb );

  Qgs3DMapSettings mapSettings;
  mapSettings.setCrs( project.crs() );
  mapSettings.setExtent( fullExtent );
  mapSettings.setLayers( { layerRgb, mLayerBuildings } );

  mapSettings.setTransformContext( project.transformContext() );
  mapSettings.setPathResolver( project.pathResolver() );
  mapSettings.setMapThemeCollection( project.mapThemeCollection() );

  QgsFlatTerrainSettings *flatTerrainSettings = new QgsFlatTerrainSettings;
  mapSettings.setTerrainSettings( flatTerrainSettings );

  std::unique_ptr<QgsTerrainGenerator> generator = flatTerrainSettings->createTerrainGenerator( Qgs3DRenderContext::fromMapSettings( &mapSettings ) );
  QVERIFY( dynamic_cast<QgsFlatTerrainGenerator *>( generator.get() )->isValid() );
  QCOMPARE( dynamic_cast<QgsFlatTerrainGenerator *>( generator.get() )->crs(), mapSettings.crs() );

  QgsPointLightSettings defaultPointLight;
  defaultPointLight.setPosition( mapSettings.origin() + QgsVector3D( 0, 400, 0 ) );
  defaultPointLight.setConstantAttenuation( 0 );
  mapSettings.setLightSources( { defaultPointLight.clone() } );
  mapSettings.setOutputDpi( 92 );

  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( mapSettings, &engine );
  engine.setRootEntity( scene );

  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 1500, 40.0, -10.0 );

  do3DSceneExport( u"flat_terrain_scene_export"_s, 1, 2, 401, 19875, scene, mLayerBuildings, &engine, scene->terrainEntity() );

  delete scene;
  mapSettings.setLayers( {} );
}

void TestQgs3DExporter::test3DSceneExporterInstanced()
{
  const QgsRectangle fullExtent( 1000, 1000, 2000, 2000 );

  auto layerPointsZ = std::make_unique<QgsVectorLayer>( "PointZ?crs=EPSG:27700", "points Z", "memory" );

  QgsPoint *p1 = new QgsPoint( 1000, 1000, 50 );
  QgsPoint *p2 = new QgsPoint( 1000, 2000, 100 );

  QgsFeature f1( layerPointsZ->fields() );
  QgsFeature f2( layerPointsZ->fields() );

  f1.setGeometry( QgsGeometry( p1 ) );
  f2.setGeometry( QgsGeometry( p2 ) );

  QgsFeatureList featureList;
  featureList << f1 << f2;
  layerPointsZ->dataProvider()->addFeatures( featureList );

  QgsPoint3DSymbol *plane3DSymbol = new QgsPoint3DSymbol();
  plane3DSymbol->setShape( Qgis::Point3DShape::Plane );
  QVariantMap vmPlane;
  vmPlane[u"size"_s] = 100.0f;
  plane3DSymbol->setShapeProperties( vmPlane );
  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::blue );
  plane3DSymbol->setMaterialSettings( materialSettings.clone() );

  layerPointsZ->setRenderer3D( new QgsVectorLayer3DRenderer( plane3DSymbol ) );

  Qgs3DMapSettings mapSettings;
  mapSettings.setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 27700 ) );
  mapSettings.setExtent( fullExtent );
  mapSettings.setLayers( QList<QgsMapLayer *>() << layerPointsZ.get() );
  mapSettings.setOutputDpi( 92 );

  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( mapSettings, &engine );

  scene->cameraController()->setLookingAtPoint( QgsVector3D( 0, 0, 0 ), 7000, 20.0, -10.0 );
  engine.setRootEntity( scene );

  do3DSceneExport( u"instanced_export"_s, 1, 2, 0, 12, scene, layerPointsZ.get(), &engine );

  delete scene;
  mapSettings.setLayers( {} );
}

void TestQgs3DExporter::doCheckFeatureZ(
  QgsFeature f,
  QVector<float> expectedZ,
  Qgs3DMapSettings *map, //
  QgsAbstract3DSymbol *symbolTerrain,
  QgsVectorLayer *layerData
)
{
  std::unique_ptr<Qt3DCore::QEntity> entity;
  Qgs3DRenderContext renderContext = Qgs3DRenderContext::fromMapSettings( map );
  renderContext.expressionContext().setFeature( f );

  QgsFeature3DHandler *handler = QgsApplication::symbol3DRegistry()->createHandlerForSymbol( layerData, symbolTerrain );
  QSet<QString> attributeNames;
  QVERIFY( handler->prepare( renderContext, attributeNames, QgsBox3D() ) );
  handler->processFeature( f, renderContext );

  entity.reset( new Qt3DCore::QEntity() );
  entity->setObjectName( "ROOT" );

  handler->finalize( entity.get(), renderContext );

  QCOMPARE( entity->children().size(), 1 );

  // search 3d entity
  for ( QObject *child : entity->children() )
  {
    Qt3DCore::QEntity *childEntity = qobject_cast<Qt3DCore::QEntity *>( child );
    if ( childEntity )
    {
      // search geometry renderer
      for ( QObject *comp : childEntity->children() )
      {
        Qt3DRender::QGeometryRenderer *childComp = qobject_cast<Qt3DRender::QGeometryRenderer *>( comp );
        if ( childComp )
        {
          QVERIFY( childComp->geometry() );

          // search position attribute
          for ( Qt3DCore::QAttribute *attrib : childComp->geometry()->attributes() )
          {
            if ( attrib->name() == Qt3DCore::QAttribute::defaultPositionAttributeName() )
            {
              QCOMPARE( attrib->count(), expectedZ.size() );
              Qt3DCore::QBuffer *buff3d = attrib->buffer();
              QByteArray buff = buff3d->data();
              const float *ptrFloat = reinterpret_cast<const float *>( buff.constData() );
              for ( uint i = 0; i < attrib->count(); ++i )
              {
                float z = ptrFloat[( static_cast<unsigned long>( i ) * attrib->byteStride() / sizeof( float ) ) + 2];
                float expZ = expectedZ[static_cast<int>( i )];
                if ( z != expZ )
                {
                  float x = ptrFloat[( static_cast<unsigned long>( i ) * attrib->byteStride() / sizeof( float ) ) + 0];
                  float y = ptrFloat[( static_cast<unsigned long>( i ) * attrib->byteStride() / sizeof( float ) ) + 1];
                  qWarning() << "Z not match at:" << x << "," << y << "! Actual[" << i << "]=" << z << "/ expected= " << expZ;
                }
                QCOMPARE( z, expZ );
              }
              break;
            }
          }
          break;
        }
      }
    }
  }
}


void TestQgs3DExporter::doCheckElevation(
  const QgsRectangle &fullExtent,
  const QgsCoordinateReferenceSystem &crs,
  const QString &dataPath,
  const QString &dtmPath,
  const QString &hiResDtmPath,
  const QgsRectangle &featExtent,
  const QVector<float> &expectedZ,
  float expectedCacheSize
)
{
  qDebug() << "=============== doCheckElevation for feature within" << featExtent.asWktCoordinates();
  QgsVectorLayer *layerData = new QgsVectorLayer( dataPath, "data", "ogr" );
  QgsRasterLayer *layerDtm = new QgsRasterLayer( dtmPath, "dtm", "gdal" );

  QgsProject project;
  project.setCrs( layerDtm->crs() );
  project.addMapLayer( layerDtm );
  project.addMapLayer( layerData );

  // =========== creating Qgs3DMapSettings
  QgsDemTerrainSettings *demTerrainSettings = new QgsDemTerrainSettings;
  demTerrainSettings->setLayer( layerDtm );
  demTerrainSettings->setVerticalScale( 3 );
  demTerrainSettings->setMapTileResolution( 16 );
  demTerrainSettings->setMaximumScreenError( 0.5 );

  Qgs3DMapSettings *mapSettings = new Qgs3DMapSettings;
  mapSettings->setLayers( QList<QgsMapLayer *>() << layerData << layerDtm );
  mapSettings->setCrs( crs );
  mapSettings->setExtent( fullExtent );
  mapSettings->setTransformContext( project.transformContext() );
  mapSettings->setPathResolver( project.pathResolver() );
  mapSettings->setMapThemeCollection( project.mapThemeCollection() );
  mapSettings->setTerrainSettings( demTerrainSettings );

  // if clamping is terrain, offset is applied
  QgsAbstract3DSymbol *symbolTerrain;
  if ( layerData->geometryType() == Qgis::GeometryType::Line )
  {
    QgsLine3DSymbol *symbol = new QgsLine3DSymbol;
    symbol->setRenderAsSimpleLines( false );
    symbol->setAltitudeClamping( Qgis::AltitudeClamping::Terrain );
    symbol->setAltitudeBinding( Qgis::AltitudeBinding::Vertex );
    symbol->setWidth( 2.0 );
    symbol->setExtrusionHeight( 0.0 );
    symbolTerrain = symbol;
  }
  else
  {
    QgsPolygon3DSymbol *symbol = new QgsPolygon3DSymbol;
    symbol->setAltitudeClamping( Qgis::AltitudeClamping::Terrain );
    symbol->setAltitudeBinding( Qgis::AltitudeBinding::Vertex );
    symbolTerrain = symbol;
  }

  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbolTerrain );
  renderer3d->setLayer( layerData );
  layerData->setRenderer3D( renderer3d );

  // build full res terrain generator
  QgsRasterLayer *hiResDtmLayer = new QgsRasterLayer( hiResDtmPath, "dtm", "gdal" );
  QgsDemTerrainGenerator hiResNoCacheDtm;
  hiResNoCacheDtm.setLayer( hiResDtmLayer );
  hiResNoCacheDtm.setCrs( mapSettings->crs(), QgsCoordinateTransformContext() );

  // =========== creating Qgs3DMapScene
  QPoint winSize = QPoint( 640, 480 ); // default window size

  QgsOffscreen3DEngine engine;
  engine.setSize( QSize( winSize.x(), winSize.y() ) );
  Qgs3DMapScene *scene = new Qgs3DMapScene( *mapSettings, &engine );
  engine.setRootEntity( scene );

  QgsFeatureIterator fi = layerData->getFeatures( featExtent );
  QgsFeature f;
  QVERIFY( fi.nextFeature( f ) );

  QgsPointXY centroid = f.geometry().centroid().asPoint();

  // =========== set camera position
  scene->cameraController()->setLookingAtPoint( QVector3D( 0, 0, 0 ), 1000, 0.0, 0.0 );

  // Calling captureSceneImage to wait for Qgs3DMapScene::Ready
  Qgs3DUtils::captureSceneImage( engine, scene );
  QCOMPARE( scene->sceneState(), Qgs3DMapScene::Ready );

  // lowest Z is computed above terrain
  float hiResNoCacheZ;
  int hiResNoCacheQ;
  const QgsTerrainGeneratorWithCache *demCache = dynamic_cast<const QgsTerrainGeneratorWithCache *>( mapSettings->terrainGenerator() );
  QVERIFY( demCache );

  for ( int x = -5; x <= 5; x += 1 )
    for ( int y = -5; y <= 5; y += 1 )
    {
      double tx = centroid.x() + x;
      double ty = centroid.y() + y;
      if ( fullExtent.contains( tx, ty ) )
      {
        hiResNoCacheZ = hiResNoCacheDtm.heightAt( tx, ty, Qgs3DRenderContext() );
        hiResNoCacheQ = hiResNoCacheDtm.qualityAt( tx, ty, Qgs3DRenderContext() );
        float z = mapSettings->terrainGenerator()->heightAt( tx, ty, Qgs3DRenderContext() );
        int q = demCache->qualityAt( tx, ty, Qgs3DRenderContext() );
        if ( std::abs( hiResNoCacheZ - z ) > 1.0 )
          qDebug()
            << "Diff in z will checking elevation at ["
            << tx
            << ty
            << "], nocache z:"
            << hiResNoCacheZ * demTerrainSettings->verticalScale()
            << //
            "(q:"
            << hiResNoCacheQ
            << ")"
            << "vs cache z:"
            << z * demTerrainSettings->verticalScale()
            << "(q:"
            << q
            << ")";
        QVERIFY2( hiResNoCacheQ < q, u"fullRezQ:%1 / q:%2"_s.arg( hiResNoCacheQ ).arg( q ).toStdString().c_str() );
      }
    }

  QCOMPARE( demCache->heightMapCache()->size(), expectedCacheSize );

  doCheckFeatureZ( f, expectedZ, mapSettings, symbolTerrain, layerData );
}


void TestQgs3DExporter::testChunkedEntityElevationDtm()
{
  doCheckElevation(
    QgsRectangle( 321875, 130109, 321930, 130390 ),
    QgsCoordinateReferenceSystem( "EPSG:27700" ),
    testDataPath( "/3d/buildings.shp" ),
    testDataPath( "/3d/dtm.tif" ),
    testDataPath( "/3d/dtm.tif" ),
    QgsRectangle( 321900, 130360, 321930, 130390 ),
    QVector<float> { 306.0f, 309.0f, 309.0f, 309.0f, 309.0f, 309.0f },
    64
  );

  doCheckElevation(
    QgsRectangle( 321875, 130109, 321930, 130390 ),
    QgsCoordinateReferenceSystem( "EPSG:27700" ),
    testDataPath( "/3d/buildings.shp" ),
    testDataPath( "/3d/dtm.tif" ),
    testDataPath( "/3d/dtm.tif" ),
    QgsRectangle( 321875, 130109, 321883, 130120 ),
    QVector<float> { 231.0f, 231.0f, 228.0f, 228.0f, 231.0f, 228.0f },
    64
  );
}

QGSTEST_MAIN( TestQgs3DExporter )
#include "testqgs3dexporter.moc"
