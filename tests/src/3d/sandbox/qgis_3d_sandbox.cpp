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
#include <QDesktopWidget>

#include "qgsapplication.h"
#include "qgs3d.h"
#include "qgslayertree.h"
#include "qgsmapsettings.h"
#include "qgspointcloudlayer.h"
#include "qgspointcloudlayer3drenderer.h"
#include "qgsproject.h"
#include "qgsflatterraingenerator.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmapcanvas.h"
#include <iostream>
#include <fstream>
#include <streambuf>
#include "qgslinestring.h"
#include "qgsvectorlayer.h"
#include "qgsline3dsymbol.h"
#include "qgssimplelinematerialsettings.h"
#include "qgsvectorlayer3drenderer.h"
#include "3dtiles/qgs3dtileschunkloader.h"

#include "3dtiles/3dtiles.h"

class RandomAmbientLineMaterialSettings : public QgsSimpleLineMaterialSettings{
//    QColor ambient() const { return QColor(random()%255, random()%255, random()%255, 255); }
    QColor ambient() const { return QColor(5, 100, 255, 255); }
    QString type() const
    {
      return QStringLiteral( "RandomAmbientLine" );
    }

};

void initCanvas3D( Qgs3DMapCanvas *canvas, Qgs3DMapSettings *mapSet, QList<QgsGeometry> & glist,
                   const QgsCoordinateTransform &coordTrans, Tileset * ts )
{
  QgsProject::instance()->setCrs(QgsCoordinateReferenceSystem::fromEpsgId(3857));
  QgsLayerTree *root = QgsProject::instance()->layerTreeRoot();
  QList< QgsMapLayer * > visibleLayers = root->checkedLayers();


  QgsVectorLayer *layerLines = new QgsVectorLayer( "LineString?crs=EPSG:3857", "lines", "memory" );

  QgsLine3DSymbol *lineSymbol = new QgsLine3DSymbol;
  lineSymbol->setRenderAsSimpleLines( true );
  lineSymbol->setWidth( 5 );
  //RandomAmbientLineMaterialSettings mat;
  QgsSimpleLineMaterialSettings mat;
  mat.setAmbient( Qt::red );
  lineSymbol->setMaterial( mat.clone() );
  layerLines->setRenderer3D( new QgsVectorLayer3DRenderer( lineSymbol ) );

  QgsFeatureList flist;
/*
  {
      Q3dCube cube (QVector3D(-1.0,-1.0,-1.0), QVector3D(1.0,1.0,1.0));
      QgsMatrix4x4 fromRotationTranslation(
                -14.70, -212.29241603812326, 9.8252767297123391, 4044595.94874227, //
                263.414, -11.849316119388336, 0.54840776747259878, 225587.8020203301, //
                2.1967831930958647e-14, 174.25030423546778, 12.00761170187392, 4910108.3100663377, //
                0.0, 0.0, 0.0, 1.0
              );
      qDebug() << "rot matrix: " << fromRotationTranslation << "\n";
      cube = fromRotationTranslation*cube;
      qDebug() << "cube: " << cube << "\n";
      qDebug() << "BBox: " << QgsGeometry (new QgsLineString (cube.asQgsPoints())).asWkt();
      QgsFeature f1( layerLines->fields() );
      f1.setGeometry( QgsGeometry (new QgsLineString (cube.asQgsPoints())) );
      flist << f1;

      QgsVector4D vtest = QgsVector4D(-1.0, -1.0, -1.0, 1.0);
      vtest = fromRotationTranslation*vtest;
      qDebug() << "vtest: " << vtest << "\n";
  }
*/
  for (QgsGeometry & g : glist) {
      QgsFeature f1( layerLines->fields() );
      f1.setGeometry( g );
      flist << f1;
  }
  qDebug() << "geom size: " << flist.length() << "\n";
  /*{
    //  Q3dCube cub(QVector3D(728036.09, 901091.51, 000.0), QVector3D(721236.09, 901991.51, 100.0));
      Q3dCube cub(QVector3D(-1000.09, -200.51, 100.0), QVector3D(500.09, 200.51, 400.0));
      QgsFeature f1( layerLines->fields() );
      f1.setGeometry( QgsGeometry (new QgsLineString (cub.asQgsPoints())) );
      flist << f1;
  }*/


  //layerLines->dataProvider()->addFeatures( flist );
  visibleLayers << layerLines;

  QgsMapSettings ms;
  ms.setDestinationCrs( QgsProject::instance()->crs() );
  ms.setLayers( visibleLayers );
  QgsRectangle fullExtent = ms.fullExtent();

  QgsRectangle extent = fullExtent;
  extent.scale( 1.3 );

  mapSet->setCrs( QgsProject::instance()->crs() );
//  map->setOrigin( QgsVector3D( fullExtent.center().x(), fullExtent.center().y(), 0 ) );
  mapSet->setOrigin( QgsVector3D( 0, 0, 0 ) );
  mapSet->setLayers( visibleLayers );
  mapSet->setTerrainLayers( visibleLayers );

  mapSet->setTransformContext( QgsProject::instance()->transformContext() );
  mapSet->setPathResolver( QgsProject::instance()->pathResolver() );
  mapSet->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );
  QObject::connect( QgsProject::instance(), &QgsProject::transformContextChanged, mapSet, [mapSet]
  {
    mapSet->setTransformContext( QgsProject::instance()->transformContext() );
  } );

  QgsFlatTerrainGenerator *flatTerrain = new QgsFlatTerrainGenerator;
  flatTerrain->setCrs( mapSet->crs() );
  flatTerrain->setExtent( fullExtent );
  mapSet->setTerrainGenerator( flatTerrain );

  QgsPointLightSettings defaultPointLight;
//  defaultPointLight.setPosition( QgsVector3D( 0, 1000, 0 ) );
//  defaultPointLight.setPosition( QgsVector3D( 728636.09, 901391.51, 1000 ) );
  defaultPointLight.setPosition( QgsVector3D( 4e6, 0.25e6, 10e6 ) );
  defaultPointLight.setConstantAttenuation( 0 );
  mapSet->setPointLights( QList<QgsPointLightSettings>() << defaultPointLight );
  mapSet->setOutputDpi( QgsApplication::desktop()->logicalDpiX() );

  canvas->setMap( mapSet );

  Qgs3dTilesChunkLoaderFactory facto(*mapSet, coordTrans, ts);
  QgsChunkNode * rootNode = facto.createRootNode();
  QgsChunkLoader * loader = facto.createChunkLoader(rootNode);
  Qt3DCore::QEntity *rootEntity = loader->createEntity(canvas->scene());


 // float dist = static_cast< float >( std::max( extent.width(), extent.height() ) );
//  canvas->setViewFromTop( extent.center(), dist * 2, 0 );
//  canvas->setViewFromTop( QgsPointXY(728636.09, 901391.51), dist * 2, 0 );

  // 4978
  //canvas->scene()->cameraController()->setViewFromTop( 44813.0f, -225335.0f, 1000, 0 );
  // 4326 4979
  // canvas->scene()->cameraController()->setViewFromTop( 0.0f, -50.0f, 500, 0 );
  // 3857
  canvas->scene()->cameraController()->setViewFromTop( 354900.00, -6561960.0, 1000.0, 0.0 );

  /*
  QgsCameraPose camPose;
  camPose.setCenterPoint( QgsVector3D( 4044595, 4910108, 225587 ) );
  camPose.setDistanceFromCenterPoint( 300 );

  // a basic setup to make frustum depth range long enough that it does not cull everything
  canvas->scene()->cameraController()->camera()->setNearPlane( 1 );
  canvas->scene()->cameraController()->camera()->setFarPlane( 1000 );
  canvas->scene()->cameraController()->setCameraPose( camPose );
*/

//  canvas->scene()->cameraController()->setViewFromTop( 0, 0, 5e6, 0 );
  //canvas->setViewFromTop( QgsPointXY( 4e6, 0.25e6), 5e6, 0 );
  canvas->scene()->cameraController()->camera()->setNearPlane(1);
  canvas->scene()->cameraController()->camera()->setFarPlane(10000.0f);
  canvas->scene()->cameraController()->camera()->setFieldOfView(90.0f);


  QObject::connect( canvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, [canvas]
  {
    qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
  } );

  QObject::connect( canvas->scene()->cameraController(), &QgsCameraController::cameraChanged, [canvas]
  {
    qDebug() << "Camera pos:" << canvas->scene()->cameraController()->camera()->position();
  } );
  qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
}


void createBboxes(Tile * ts, const QgsCoordinateTransform & coordTrans, QList<QgsGeometry> & glist) {
    QgsGeometry cube = ts->getBoundingVolumeAsGeometry(&coordTrans);
    // 4978
    //cube.translate(-4000000, 0, -4909500);
    // 3857
    //cube.translate(0.0, 0000000.0, 0);

    //cube.transform(coordTrans);
    qDebug() << "Tile BBox: " << cube.asWkt();
    glist << cube;
    for (std::shared_ptr<Tile> t : ts->mChildren) {
        createBboxes(t.get(), coordTrans, glist);
    }
}

int main( int argc, char *argv[] )
{
  QApplication app( argc, argv );

  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  if ( argc < 3 )
  {
    qDebug() << "need json_tileset_file QGIS_project_file";
    return 1;
  }

  QString jsonFile(argv[1]);
  QString projectFile(argv[2]);

  Qgs3DMapSettings *map = new Qgs3DMapSettings;

  qDebug() << "Will read json: " << jsonFile << "\n";
  qDebug() << "Will read qgis: " << projectFile << "\n";
  std::unique_ptr<ThreeDTilesContent> ts = Tileset::fromUrl(QUrl(jsonFile));

  //qDebug() << "Dump: \n" << *ts.get()<< "\n";

  bool res = QgsProject::instance()->read( projectFile );
  if ( !res )
  {
    qDebug() << "can't open project file" << projectFile;
    return 1;
  }

  /* Suggested pipeline:
  QgsCoordinateReferenceSystem destCoord = QgsCoordinateReferenceSystem::fromProj(
              QString("proj=pipeline\n") //
              + "proj=geocent +datum=WGS84 +units=m +no_defs\n" //
              + "step proj=cart ellps=intl\n" //
              + "step proj=helmert convention=coordinate_frame\n" //
              + "  x=-81.0703  y=-89.3603  z=-115.7526 \n" //
              + "  rx=-0.48488 ry=-0.02436 rz=-0.41321  s=-0.540645\n" //
              + "step proj=cart inv ellps=GRS80 \n" //
              + "step proj=longlat +datum=WGS84 +no_defs +vto_meter\n" //
              );
*/
  QgsCoordinateTransform coordTrans(QgsCoordinateReferenceSystem::fromEpsgId(4978),
                                    QgsCoordinateReferenceSystem::fromEpsgId(3857),
                                    QgsProject::instance());
  qDebug() << "2154: " << QgsCoordinateReferenceSystem::fromEpsgId(2154).toProj();
  qDebug() << "3857: " << QgsCoordinateReferenceSystem::fromEpsgId(3857).toProj();
  qDebug() << "4326: " << QgsCoordinateReferenceSystem::fromEpsgId(4326).toProj();
  qDebug() << "4979: " << QgsCoordinateReferenceSystem::fromEpsgId(4979).toProj();
  qDebug() << "4978: " << QgsCoordinateReferenceSystem::fromEpsgId(4978).toProj();

  QList<QgsGeometry> glist;
  createBboxes(((Tileset*)ts.get())->mRoot.get(), coordTrans, glist);

  // a hack to assign 3D renderer
  for ( QgsMapLayer *layer : QgsProject::instance()->layerTreeRoot()->checkedLayers() )
  {
    if ( QgsPointCloudLayer *pcLayer = qobject_cast<QgsPointCloudLayer *>( layer ) )
    {
      QgsPointCloudLayer3DRenderer *r = new QgsPointCloudLayer3DRenderer();
      r->setLayer( pcLayer );
      r->resolveReferences( *QgsProject::instance() );
      pcLayer->setRenderer3D( r );
    }
  }



  Qgs3DMapCanvas *canvas = new Qgs3DMapCanvas;
  initCanvas3D( canvas, map, glist, coordTrans,  (Tileset *)ts.get());
  canvas->resize( 1200, 1000 );
  canvas->show();

  return app.exec();
}
