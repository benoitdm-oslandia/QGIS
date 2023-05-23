/***************************************************************************
  testqgselevationProfile.cpp
  --------------------------------------
Date                 : August 2022
Copyright            : (C) 2022 by Martin Dobias
Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgstest.h"

#include "qgsapplication.h"
#include "qgsvectorlayerprofilegenerator.h"
#include "qgsprofilerequest.h"
#include "qgscurve.h"
#include "qgsvectorlayer.h"
#include <qgsproject.h>


class TestQgsElevationProfile : public QgsTest
{
    Q_OBJECT

  public:
    TestQgsElevationProfile() : QgsTest( QStringLiteral( "Elevation Profile Tests" ), QStringLiteral( "elevation_Profile" ) ) {}

  private:
    QgsVectorLayer *mpPointsLayer = nullptr;
    QgsVectorLayer *mpLinesLayer = nullptr;
    QgsVectorLayer *mpPolygonsLayer = nullptr;
    QString mTestDataDir;
    QgsPointSequence mProfilePoints;
    QgsVectorLayerProfileResults *mProfileResults = nullptr;

    void doCheckPoint( QgsProfileRequest &request, int tolerance, QgsVectorLayer *layer,
                       const QList<QgsFeatureId> &expectedFeatures );
    void doCheckLine( QgsProfileRequest &request, int tolerance, QgsVectorLayer *layer,
                      const QList<QgsFeatureId> &expectedFeatures, const QList<int> &nbSubGeomPerFeature );

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void testVectorLayerProfileForPoint();
    void testVectorLayerProfileForLine();

};


void TestQgsElevationProfile::initTestCase()
{
  //
  // Runs once before any tests are run
  //
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  QgsApplication::showSettings();

  QgsProject::instance()->setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 3857 ) );

  //create some objects that will be used in all tests...

  const QString myDataDir( TEST_DATA_DIR ); //defined in CmakeLists.txt
  mTestDataDir = myDataDir + "/3d/";

  //
  // Create a line layer that will be used in all tests...
  //
  const QString myLinesFileName = mTestDataDir + "lines.shp";
  const QFileInfo myLineFileInfo( myLinesFileName );
  mpLinesLayer = new QgsVectorLayer( myLineFileInfo.filePath(),
                                     myLineFileInfo.completeBaseName(), QStringLiteral( "ogr" ) );
  // Register the layer with the registry
  QgsProject::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mpLinesLayer );

  //
  // Create a point layer that will be used in all tests...
  //
  const QString myPointsFileName = mTestDataDir + "points_with_z.shp";
  const QFileInfo myPointFileInfo( myPointsFileName );
  mpPointsLayer = new QgsVectorLayer( myPointFileInfo.filePath(),
                                      myPointFileInfo.completeBaseName(), QStringLiteral( "ogr" ) );
  // Register the layer with the registry
  QgsProject::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mpPointsLayer );

  //
  // Create a polygon layer that will be used in all tests...
  //
  const QString myPolygonsFileName = mTestDataDir + "buildings.shp";
  const QFileInfo myPolygonFileInfo( myPolygonsFileName );
  mpPolygonsLayer = new QgsVectorLayer( myPolygonFileInfo.filePath(),
                                        myPolygonFileInfo.completeBaseName(), QStringLiteral( "ogr" ) );
  // Register the layer with the registry
  QgsProject::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mpPolygonsLayer );

  //initial adding of geometry should set z/m type
//  mProfilePoints << QgsPoint( Qgis::WkbType::Point, -347830, 6632930 )
//                 << QgsPoint( Qgis::WkbType::Point, -346160, 6632030 )
//                 << QgsPoint( Qgis::WkbType::Point, -346390, 6631620 )
//                 << QgsPoint( Qgis::WkbType::Point, -346630, 6632140 ) ;
  mProfilePoints << QgsPoint( Qgis::WkbType::Point, -346120, 6631840 )
                 << QgsPoint( Qgis::WkbType::Point, -346550, 6632030 )
                 << QgsPoint( Qgis::WkbType::Point, -346440, 6632140 )
                 << QgsPoint( Qgis::WkbType::Point, -347830, 6632930 ) ;


}

void TestQgsElevationProfile::cleanupTestCase()
{
  QgsApplication::exitQgis();
}

void TestQgsElevationProfile::doCheckPoint( QgsProfileRequest &request, int tolerance, QgsVectorLayer *layer,
    const QList<QgsFeatureId> &expectedFeatures )
{
  request.setTolerance( tolerance );

  QgsAbstractProfileGenerator *profGen = layer->createProfileGenerator( request );
  QVERIFY( profGen );
  QVERIFY( profGen->generateProfile() );
  mProfileResults = dynamic_cast<QgsVectorLayerProfileResults *>( profGen->takeResults() );
  QVERIFY( mProfileResults );
  QVERIFY( ! mProfileResults->features.empty() );

  QList<QgsFeatureId> expected = expectedFeatures;
  std::sort( expected.begin(), expected.end() );

  QList<QgsFeatureId> actual = mProfileResults->features.keys();
  std::sort( actual.begin(), actual.end() );
  qDebug() << actual;

  QCOMPARE( actual, expected );
}

void TestQgsElevationProfile::doCheckLine( QgsProfileRequest &request, int tolerance, QgsVectorLayer *layer,
    const QList<QgsFeatureId> &expectedFeatures, const QList<int> &nbSubGeomPerFeature )
{
  doCheckPoint( request, tolerance, layer, expectedFeatures );

  int i = 0;
  for ( auto it = mProfileResults->features.constBegin();
        it != mProfileResults->features.constEnd(); ++it, ++i )
  {
    for ( const QgsVectorLayerProfileResults::Feature &feat : it.value() )
    {
      qDebug() << "feat:" << feat.featureId << "geom:" << feat.geometry;
    }
    QCOMPARE( it.value().size(), nbSubGeomPerFeature[i] );
  }
}

void TestQgsElevationProfile::testVectorLayerProfileForPoint()
{
  QgsLineString *profileCurve = new QgsLineString ;
  profileCurve->setPoints( mProfilePoints );

  QgsProfileRequest request( profileCurve );
  request.setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 3857 ) );

  doCheckPoint( request, 15, mpPointsLayer, { 5, 11, 12, 13, 14, 15, 18, 45, 46 } );
  doCheckPoint( request, 70, mpPointsLayer, { 0, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 45, 46, 48 } );
}


void TestQgsElevationProfile::testVectorLayerProfileForLine()
{
  QgsLineString *profileCurve = new QgsLineString ;
  profileCurve->setPoints( mProfilePoints );

  QgsProfileRequest request( profileCurve );
  request.setCrs( QgsCoordinateReferenceSystem::fromEpsgId( 3857 ) );

  doCheckLine( request, 1, mpLinesLayer, { 0, 2 }, { 1, 5 } );
  doCheckLine( request, 20, mpLinesLayer, { 0, 2 }, { 1, 3 } );
  doCheckLine( request, 50, mpLinesLayer, { 1, 0, 2 }, { 1, 1, 1 } );
}


QGSTEST_MAIN( TestQgsElevationProfile )
#include "testqgselevationprofile.moc"
