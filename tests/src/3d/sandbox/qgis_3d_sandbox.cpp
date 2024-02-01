/***************************************************************************
                         qgis_3d_sandbox.cpp
                         --------------------
    begin                : October 2020
    copyright            : (C) 2020 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>

#include "qgsapplication.h"
#include "qgs3d.h"
#include "qgslayertree.h"
#include "qgsmapsettings.h"
//#include "qgspointcloudlayer.h"
//#include "qgspointcloudlayer3drenderer.h"
#include "qgsproject.h"
#include "qgsflatterraingenerator.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmapcanvas.h"
#include "qgsprojectelevationproperties.h"
#include "qgsprojectviewsettings.h"
#include "qgspointlightsettings.h"
#include "qgsterrainprovider.h"
//#include "qgstiledscenelayer.h"
//#include "qgstiledscenelayer3drenderer.h"
#include "qgsdemterraingenerator.h"
#include "qgsvectorlayer.h"
#include "qgspolygon3dsymbol.h"
#include "qgsvectorlayer3drenderer.h"

#include <QScreen>

/**
     * Returns the full path to the test data with the given file path.
     */
QString testDataPath( const QString &filePath )
{
  return QDir( QStringLiteral( TEST_DATA_DIR ) + '/' ).filePath( filePath.startsWith( '/' ) ? filePath.mid( 1 ) : filePath );
}

void initCanvas3D( Qgs3DMapCanvas *canvas )
{
  QgsCoordinateReferenceSystem crs = QgsProject::instance()->crs();
  if ( crs.isGeographic() )
  {
    // we can't deal with non-projected CRS, so let's just pick something
    QgsProject::instance()->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  }

  QgsVectorLayer *layerBuildings = new QgsVectorLayer( testDataPath( "/3d/buildings.shp" ), "buildings", "ogr" );
  QgsProject::instance()->addMapLayer( layerBuildings );
  QgsPhongMaterialSettings materialSettings;
  materialSettings.setAmbient( Qt::lightGray );
  QgsPolygon3DSymbol *symbol3d = new QgsPolygon3DSymbol;
  symbol3d->setMaterialSettings( materialSettings.clone() );
  symbol3d->setExtrusionHeight( 10.f );
  QgsVectorLayer3DRenderer *renderer3d = new QgsVectorLayer3DRenderer( symbol3d );
  layerBuildings->setRenderer3D( renderer3d );

  QgsRasterLayer *layerDtm = new QgsRasterLayer( testDataPath( "/3d/dtm.tif" ), "dtm", "gdal" );
  QgsProject::instance()->addMapLayer( layerDtm );

  QgsMapSettings ms;
  ms.setDestinationCrs( QgsProject::instance()->crs() );
  ms.setLayers( {layerBuildings, layerDtm} );
  QgsRectangle fullExtent = QgsProject::instance()->viewSettings()->fullExtent();

  Qgs3DMapSettings *mapSettings = new Qgs3DMapSettings;
  mapSettings->setCrs( QgsProject::instance()->crs() );
  mapSettings->setOrigin( QgsVector3D( fullExtent.center().x(), fullExtent.center().y(), 0 ) );
  mapSettings->setLayers( {layerBuildings, layerDtm} );

  mapSettings->setExtent( fullExtent );

  Qgs3DAxisSettings axis;
  axis.setMode( Qgs3DAxisSettings::Mode::Off );
  mapSettings->set3DAxisSettings( axis );

  mapSettings->setTransformContext( QgsProject::instance()->transformContext() );
  mapSettings->setPathResolver( QgsProject::instance()->pathResolver() );
  mapSettings->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );
  QObject::connect( QgsProject::instance(), &QgsProject::transformContextChanged, mapSettings, [mapSettings]
  {
    mapSettings->setTransformContext( QgsProject::instance()->transformContext() );
  } );

  QgsDemTerrainGenerator *demTerrain = new QgsDemTerrainGenerator();
  demTerrain->setLayer( layerDtm );
  mapSettings->setTerrainGenerator( demTerrain );
  mapSettings->setTerrainVerticalScale( 3 );

  QgsPointLightSettings defaultPointLight;
  defaultPointLight.setPosition( QgsVector3D( 0, 400, 0 ) );
  defaultPointLight.setConstantAttenuation( 0 );
  mapSettings->setLightSources( {defaultPointLight.clone() } );

  if ( QScreen *screen = QGuiApplication::primaryScreen() )
    mapSettings->setOutputDpi( screen->physicalDotsPerInch() );
  else
    mapSettings->setOutputDpi( 96 );

  mapSettings->setDebugDepthMapSettings( true, Qt::Corner::BottomRightCorner, 0.5 );

  canvas->setMapSettings( mapSettings );
  canvas->cameraController()->setLookingAtPoint( QVector3D( 0, 0, 0 ), 1500, 40.0, -10.0 );

  QObject::connect( canvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, canvas, [canvas]
  {
    qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
  } );

  qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
}

int main( int argc, char *argv[] )
{
  QgsApplication myApp( argc, argv, true, QString(), QStringLiteral( "desktop" ) );

  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

//  if ( argc < 2 )
//  {
//    qDebug() << "need QGIS project file";
//    return 1;
//  }

//  const QString projectFile = argv[1];
//  const bool res = QgsProject::instance()->read( projectFile );
//  if ( !res )
//  {
//    qDebug() << "can't open project file" << projectFile;
//    return 1;
//  }

//  // a hack to assign 3D renderer
//  for ( QgsMapLayer *layer : QgsProject::instance()->layerTreeRoot()->checkedLayers() )
//  {
//    if ( QgsPointCloudLayer *pcLayer = qobject_cast<QgsPointCloudLayer *>( layer ) )
//    {
//      QgsPointCloudLayer3DRenderer *r = new QgsPointCloudLayer3DRenderer();
//      r->setLayer( pcLayer );
//      r->resolveReferences( *QgsProject::instance() );
//      pcLayer->setRenderer3D( r );
//    }

//    if ( QgsTiledSceneLayer *tsLayer = qobject_cast<QgsTiledSceneLayer *>( layer ) )
//    {
//      QgsTiledSceneLayer3DRenderer *r = new QgsTiledSceneLayer3DRenderer();
//      r->setLayer( tsLayer );
//      r->resolveReferences( *QgsProject::instance() );
//      tsLayer->setRenderer3D( r );
//    }
//  }

  Qgs3DMapCanvas *canvas = new Qgs3DMapCanvas;
  initCanvas3D( canvas );
  canvas->resize( 800, 600 );
  canvas->show();

  return myApp.exec();
}
