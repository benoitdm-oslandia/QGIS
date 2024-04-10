#include "qgssfcgalengine.h"
#include "qgsgeometry.h"
#include "qgspolygon.h"
#include "qgsgeometryfactory.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <charconv>
#include "qgssfcgalgeometry.h"

// ===================================
// sfcgal namespace
// ===================================

Q_GLOBAL_STATIC( sfcgal::ErrorHandler, _sfcgalErrorHandler )

sfcgal::ErrorHandler *sfcgal::errorHandler()
{
  return _sfcgalErrorHandler;
}


void sfcgal::Deleter::operator()( sfcgal_geometry_t *geom ) const
{
  sfcgal_geometry_delete( geom );
}

void sfcgal::Deleter::operator()( const sfcgal_prepared_geometry_t *geom ) const
{
  sfcgal_prepared_geometry_delete( ( sfcgal_prepared_geometry_t * )geom );
}

sfcgal::shared_ptr sfcgal::make_shared_ptr( sfcgal_geometry_t *geom )
{
  return sfcgal::shared_ptr( geom, sfcgal::Deleter() );
}

bool sfcgal::ErrorHandler::hasFailed( QString *errorMsg )
{
  if ( !getText().isEmpty() )
  {
    QgsDebugError( QString( "sfcgal operation failed: " ) + getText() );
    if ( errorMsg )
    {
      errorMsg->clear();
      errorMsg->append( getText() );
    }
    return true;
  }
  return false;
}

int sfcgal::errorCallback( const char *fmt, ... )
{
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  sfcgal::errorHandler()->addText( QStringLiteral( "SFCgal error occurred: %1" ).arg( buffer ) );

  return strlen( buffer );
}

int sfcgal::warningCallback( const char *fmt, ... )
{
  va_list ap;
  char buffer[1024];

  va_start( ap, fmt );
  vsnprintf( buffer, sizeof buffer, fmt, ap );
  va_end( ap );

  sfcgal::errorHandler()->addText( QStringLiteral( "SFCgal warning occurred: %1" ).arg( buffer ) );

  return strlen( buffer );
}


sfcgal::ErrorHandler::ErrorHandler()
{
  sfcgal_init(); // empty but called
  sfcgal_set_error_handlers( sfcgal::warningCallback, sfcgal::errorCallback );
}

void sfcgal::ErrorHandler::clearText()
{
  errorMessage.clear();
}

QString sfcgal::ErrorHandler::getText() const
{
  return errorMessage;
}

void sfcgal::ErrorHandler::addText( const QString &msg )
{
  qDebug() << "sfcgal::Handler::addErrorText" << msg;
  errorMessage.append( msg );
}

// Release return with delete []
unsigned char *sfcgal::hex2bytes( const char *hex, int *size )
{
  QByteArray ba = QByteArray::fromHex( hex );
  unsigned char *out = new unsigned char[ba.size()];
  memcpy( out, ba.data(), ba.size() );
  *size = ba.size();
  return out;
}

QString sfcgal::bytes2hex( const char *bytes, int size )
{
  QByteArray ba( bytes, size );
  QString out = ba.toHex();
  return out;
}

// ===================================
// QgsSfcgalEngine class
// ===================================

QgsSfcgalEngine::QgsSfcgalEngine( const QgsAbstractGeometry *geometry, double precision )
  : QgsGeometryEngine( geometry )
  , mPrecision( precision )
{
  mSfcgalGeom = fromAbsGeometry( geometry );
  QString errorMsg;
  if ( sfcgal::errorHandler()->hasFailed( &errorMsg ) )
    QgsDebugError( QStringLiteral( "Unable to init sfcgal with qgs geom. Error: %1" ).arg( errorMsg ) );
}

QgsSfcgalEngine::QgsSfcgalEngine( sfcgal::shared_ptr geometry, double precision )
  : QgsGeometryEngine( nullptr )
  , mPrecision( precision )
{
  mSfcgalGeom = geometry;
  mGeometry = toAbsGeometry( mSfcgalGeom.get() ).release();
  QString errorMsg;
  if ( sfcgal::errorHandler()->hasFailed( &errorMsg ) )
    QgsDebugError( QStringLiteral( "Unable to init sfcgal with sfcgal geom. Error: %1" ).arg( errorMsg ) );
}


std::unique_ptr<QgsAbstractGeometry> QgsSfcgalEngine::toAbsGeometry( const sfcgal::geometry *geom )
{
  sfcgal::errorHandler()->clearText();
  std::unique_ptr<QgsAbstractGeometry> out;
  char *wkbHex;
  size_t len = 0;
  sfcgal_geometry_as_wkb( geom, &wkbHex, &len );
  if ( sfcgal::errorHandler()->hasFailed() )
    return nullptr;

  wkbHex [len] = 0; // temp fix bad ended array

  int byteLen = 0;
  unsigned char *wkbBytesData = sfcgal::hex2bytes( wkbHex, &byteLen );

  QgsConstWkbPtr ptr( ( const unsigned char * )wkbBytesData, byteLen );
  out = QgsGeometryFactory::geomFromWkb( ptr );
  if ( !out )
  {
    QgsConstWkbPtr ptrError( ( const unsigned char * )wkbBytesData, byteLen );
    QgsDebugError( QStringLiteral( "Wkb contains unmanaged geometry type %1" ).arg( static_cast<int>( ptrError.readHeader() ) ) );
  }
  delete [] wkbBytesData;

  return out;
}

std::unique_ptr<QgsPolygon> QgsSfcgalEngine::toPolygon( const sfcgal::geometry *geom )
{
  std::unique_ptr<QgsPolygon> out;

  sfcgal_geometry_type_t type = sfcgal_geometry_type_id( geom );
  if ( type == SFCGAL_TYPE_POLYGON || type == SFCGAL_TYPE_MULTIPOLYGON )
  {
    out.reset( qgsgeometry_cast<QgsPolygon *>( toAbsGeometry( geom ).release() ) );
  }

  return out;
}

sfcgal::shared_ptr QgsSfcgalEngine::fromAbsGeometry( const QgsAbstractGeometry *geom, QString *errorMsg )
{
  const QgsSfcgalGeometry *sfcgalGeometry = dynamic_cast<const QgsSfcgalGeometry *>( geom );
  if ( sfcgalGeometry )
    return sfcgalGeometry->sfcgalGeometry();
  else
  {
    sfcgal::errorHandler()->clearText();
    QByteArray wkbBytes = geom->asWkb();
    QString wkbHex = sfcgal::bytes2hex( wkbBytes.data(), wkbBytes.size() );

    sfcgal::geometry *out = sfcgal_io_read_wkb( wkbHex.toStdString().c_str(), wkbHex.length() );

    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;

    /*  sfcgal_geometry_is_valid(out);
    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;
    */

    return sfcgal::make_shared_ptr( out );
  }
}

sfcgal::shared_ptr QgsSfcgalEngine::cloneGeometry( const sfcgal_geometry_t *geom )
{
  sfcgal::geometry *out = sfcgal_geometry_clone( geom );
  return sfcgal::make_shared_ptr( out );
}


sfcgal::shared_ptr QgsSfcgalEngine::fromWkb( const QgsConstWkbPtr &wkbPtr, QString *errorMsg )
{
  sfcgal::errorHandler()->clearText();

  const unsigned char *wkbUnsignedPtr = wkbPtr;
  sfcgal::geometry *out = sfcgal_io_read_wkb( reinterpret_cast<const char *>( wkbUnsignedPtr ), wkbPtr.remaining() );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  return sfcgal::make_shared_ptr( out );
}

sfcgal::shared_ptr QgsSfcgalEngine::fromWkt( const QString &wkt, QString *errorMsg )
{
  sfcgal::errorHandler()->clearText();

  sfcgal::geometry *out = sfcgal_io_read_wkt( wkt.toStdString().c_str(), wkt.length() );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  return sfcgal::unique_ptr( out );
}

QgsConstWkbPtr QgsSfcgalEngine::toWkb( const sfcgal_geometry_t *geom, QString *errorMsg )
{
  sfcgal::errorHandler()->clearText();

  std::unique_ptr<QgsAbstractGeometry> out;
  char *wkbHex;
  size_t len = 0;
  sfcgal_geometry_as_wkb( geom, &wkbHex, &len );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return QgsConstWkbPtr( nullptr, 0 );


  return QgsConstWkbPtr( reinterpret_cast<unsigned char *>( wkbHex ), len );
}

QString QgsSfcgalEngine::toWkt( const sfcgal_geometry_t *geom, QString *errorMsg )
{
  sfcgal::errorHandler()->clearText();

  std::unique_ptr<QgsAbstractGeometry> out;
  char *wkt;
  size_t len = 0;
  sfcgal_geometry_as_text( geom, &wkt, &len );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return QString();

  return QString::fromStdString( std::string( wkt, len ) );
}

Qgis::WkbType QgsSfcgalEngine::wkbType( const sfcgal_geometry_t *geom )
{
  sfcgal_geometry_type_t type = sfcgal_geometry_type_id( geom ) ;

  switch ( type )
  {
    case SFCGAL_TYPE_POINT:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::PointZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::PointZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::PointM;
      return Qgis::WkbType::Point;

    case SFCGAL_TYPE_LINESTRING:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::LineStringZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::LineStringZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::LineStringM;
      return Qgis::WkbType::LineString;

    case SFCGAL_TYPE_POLYGON:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::PolygonZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::PolygonZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::PolygonM;
      return Qgis::WkbType::Polygon;

    case SFCGAL_TYPE_MULTIPOINT:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiPointZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::MultiPointZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiPointM;
      return Qgis::WkbType::MultiPoint;

    case SFCGAL_TYPE_MULTILINESTRING:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiLineStringZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::MultiLineStringZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiLineStringM;
      return Qgis::WkbType::MultiLineString;

    case SFCGAL_TYPE_MULTIPOLYGON:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiPolygonZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::MultiPolygonZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiPolygonM;
      return Qgis::WkbType::MultiPolygon;

    case SFCGAL_TYPE_GEOMETRYCOLLECTION:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::GeometryCollectionZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::GeometryCollectionZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::GeometryCollectionM;
      return Qgis::WkbType::GeometryCollection;

    case SFCGAL_TYPE_POLYHEDRALSURFACE:
    case SFCGAL_TYPE_TRIANGULATEDSURFACE:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiSurfaceZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::MultiSurfaceZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::MultiSurfaceM;
      return Qgis::WkbType::MultiSurface;

    case SFCGAL_TYPE_TRIANGLE:
      if ( sfcgal_geometry_is_3d( geom ) && sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::TriangleZM;
      if ( sfcgal_geometry_is_3d( geom ) )
        return Qgis::WkbType::TriangleZ;
      if ( sfcgal_geometry_is_measured( geom ) )
        return Qgis::WkbType::TriangleM;
      return Qgis::WkbType::Triangle;

    case SFCGAL_TYPE_SOLID:
      return Qgis::WkbType::Unknown;


    case SFCGAL_TYPE_MULTISOLID:
      return Qgis::WkbType::Unknown;
  }

  return Qgis::WkbType::Unknown;
}



void QgsSfcgalEngine::geometryChanged()
{
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void QgsSfcgalEngine::prepareGeometry()
{
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

QgsAbstractGeometry *QgsSfcgalEngine::intersection( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal::shared_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr result = QgsSfcgalEngine::intersection( mSfcgalGeom.get(), other.get(), errorMsg );

  if ( !result || !result.get() )
    return nullptr;

  return toSfcgalGeometry( result ).release();
}

sfcgal::shared_ptr QgsSfcgalEngine::intersection( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg, const QgsGeometryParameters & )
{
  sfcgal::errorHandler()->clearText();

  sfcgal_geometry_t *result = nullptr;
  if ( sfcgal_geometry_is_3d( geomA ) || sfcgal_geometry_is_3d( geomB ) )
    result = sfcgal_geometry_intersection_3d( geomA, geomB );
  else
    result = sfcgal_geometry_intersection( geomA, geomB );

  if ( !result )
    sfcgal::errorHandler()->addText( "sfcgal produced null result." );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  return sfcgal::make_shared_ptr( result );
}

QgsAbstractGeometry *QgsSfcgalEngine::difference( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal::shared_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr result = QgsSfcgalEngine::difference( mSfcgalGeom.get(), other.get(), errorMsg );

  if ( !result )
    return nullptr;

  return toSfcgalGeometry( result ).release();
}

sfcgal::shared_ptr QgsSfcgalEngine::difference( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg, const QgsGeometryParameters & )
{
  sfcgal::errorHandler()->clearText();

  sfcgal_geometry_t *result = nullptr;
  if ( sfcgal_geometry_is_3d( geomA ) || sfcgal_geometry_is_3d( geomB ) )
    result = sfcgal_geometry_difference_3d( geomA, geomB );
  else
    result = sfcgal_geometry_difference( geomA, geomB );

  if ( !result )
    sfcgal::errorHandler()->addText( "sfcgal produced null result." );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  return sfcgal::unique_ptr( result );
}


QgsAbstractGeometry *QgsSfcgalEngine::combine( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal::shared_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal_geometry_t *result = nullptr;
  if ( geom->is3D() || mGeometry->is3D() )
    result = sfcgal_geometry_union_3d( mSfcgalGeom.get(), other.get() );
  else
    result = sfcgal_geometry_union( mSfcgalGeom.get(), other.get() );

  if ( !result )
    sfcgal::errorHandler()->addText( "sfcgal produced null result." );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( result );
  return toSfcgalGeometry( sharedResult ).release();
}

QgsAbstractGeometry *QgsSfcgalEngine::combine( const QVector<QgsAbstractGeometry *> &geomList, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *combined = nullptr;
  for ( auto otherGeom : geomList )
  {
    sfcgal::shared_ptr other = fromAbsGeometry( otherGeom, errorMsg );
    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;

    if ( ! combined )
      combined = mSfcgalGeom.get();

    if ( otherGeom->is3D() || mGeometry->is3D() )
      combined = sfcgal_geometry_union_3d( combined, other.get() ) ;
    else
      combined = sfcgal_geometry_union( combined, other.get() ) ;

    if ( !combined )
      sfcgal::errorHandler()->addText( "sfcgal produced null result." );

    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;
  }

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( combined );
  return toSfcgalGeometry( sharedResult ).release();
}

QgsAbstractGeometry *QgsSfcgalEngine::combine( const QVector<QgsGeometry> &geomList, QString *errorMsg, const QgsGeometryParameters & ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *combined = nullptr;
  for ( QgsGeometry otherGeom : geomList )
  {
    sfcgal::shared_ptr other = fromAbsGeometry( otherGeom.get() );
    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;

    if ( ! combined )
      combined = mSfcgalGeom.get();

    if ( otherGeom.get()->is3D() || mGeometry->is3D() )
      combined = sfcgal_geometry_union_3d( combined, other.get() ) ;
    else
      combined = sfcgal_geometry_union( combined, other.get() ) ;

    if ( !combined )
      sfcgal::errorHandler()->addText( "sfcgal produced null result." );

    if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
      return nullptr;
  }

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( combined );
  return toSfcgalGeometry( sharedResult ).release();
}

QgsAbstractGeometry *QgsSfcgalEngine::symDifference( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters &parameters ) const
{
  Q_UNUSED( parameters );

  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::buffer( double distance, int, QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *result = nullptr;
  result = sfcgal_geometry_round( mSfcgalGeom.get(), distance ) ;

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( result );
  return toSfcgalGeometry( sharedResult ).release();
}

QgsAbstractGeometry *QgsSfcgalEngine::buffer( double distance, int, Qgis::EndCapStyle, Qgis::JoinStyle, double, QString *errorMsg ) const
{
  return buffer( distance, 0, errorMsg );
}

QgsAbstractGeometry *QgsSfcgalEngine::simplify( double tolerance, QString *errorMsg ) const
{
  Q_UNUSED( tolerance );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::interpolate( double distance, QString *errorMsg ) const
{
  Q_UNUSED( distance );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return nullptr;
}

QgsAbstractGeometry *QgsSfcgalEngine::envelope( QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *result = nullptr;
  if ( mGeometry->is3D() )
    result = sfcgal_geometry_convexhull( mSfcgalGeom.get() ) ;
  else
    result = sfcgal_geometry_convexhull_3d( mSfcgalGeom.get() ) ;

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( result );
  return toSfcgalGeometry( sharedResult ).release();
}

QgsPoint *QgsSfcgalEngine::centroid( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  return nullptr;

}

QgsPoint *QgsSfcgalEngine::pointOnSurface( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  return nullptr;

}

QgsAbstractGeometry *QgsSfcgalEngine::convexHull( QString *errorMsg ) const
{
  return envelope( errorMsg );
}

double QgsSfcgalEngine::distance( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  double result = std::numeric_limits<double>::quiet_NaN();
  sfcgal::errorHandler()->clearText();
  sfcgal::shared_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return result;

  if ( mGeometry->is3D() )
    result = sfcgal_geometry_distance_3d( mSfcgalGeom.get(), other.get() ) ;
  else
    result = sfcgal_geometry_distance( mSfcgalGeom.get(), other.get() ) ;

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return std::numeric_limits<double>::quiet_NaN();

  return result;
}

bool QgsSfcgalEngine::distanceWithin( const QgsAbstractGeometry *geom, double maxdistance, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  Q_UNUSED( maxdistance );
  return false;
}

bool QgsSfcgalEngine::intersects( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal::shared_ptr other = fromAbsGeometry( geom, errorMsg );
  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return false;

  return QgsSfcgalEngine::intersects( mSfcgalGeom.get(), other.get(), errorMsg );
}

bool QgsSfcgalEngine::intersects( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg )
{
  sfcgal::errorHandler()->clearText();

  bool result = false;
  if ( sfcgal_geometry_is_3d( geomA ) || sfcgal_geometry_is_3d( geomB ) )
    result = sfcgal_geometry_intersects_3d( geomA, geomB );
  else
    result = sfcgal_geometry_intersects( geomA, geomB );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return false;

  return result;
}

bool QgsSfcgalEngine::touches( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::crosses( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::within( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::overlaps( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::contains( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  return false;
}

bool QgsSfcgalEngine::disjoint( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

QString QgsSfcgalEngine::relate( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return "false";
}

bool QgsSfcgalEngine::relatePattern( const QgsAbstractGeometry *geom, const QString &pattern, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );
  Q_UNUSED( pattern );

  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

double QgsSfcgalEngine::area( QString *errorMsg ) const
{
  double result = std::numeric_limits<double>::quiet_NaN();
  sfcgal::errorHandler()->clearText();

  if ( mGeometry->is3D() )
    result = sfcgal_geometry_area_3d( mSfcgalGeom.get() ) ;
  else
    result = sfcgal_geometry_area( mSfcgalGeom.get() ) ;

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return std::numeric_limits<double>::quiet_NaN();

  return result;
}

double QgsSfcgalEngine::length( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return 0.0;
}

bool QgsSfcgalEngine::isValid( QString *errorMsg, bool, QgsGeometry *errorLoc ) const
{
  sfcgal::errorHandler()->clearText();
  bool result = false;
  char *reason;
  sfcgal_geometry_t *location;
  result = sfcgal_geometry_is_valid_detail( mSfcgalGeom.get(), &reason, &location );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return false;

  if ( strlen( reason ) )
    sfcgal::errorHandler()->addText( QString( reason ) );

  if ( location && errorLoc )
  {
    std::unique_ptr<QgsAbstractGeometry> locationGeom = toAbsGeometry( location );
    errorLoc->addPart( locationGeom.release() );
  }

  return result;
}

bool QgsSfcgalEngine::isEqual( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  Q_UNUSED( geom );
  Q_UNUSED( errorMsg );

  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

bool QgsSfcgalEngine::isEmpty( QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  bool result = false;

  result = sfcgal_geometry_is_empty( mSfcgalGeom.get() );

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return false;

  return result;
}

bool QgsSfcgalEngine::isSimple( QString *errorMsg ) const
{
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return false;
}

QgsGeometryEngine::EngineOperationResult QgsSfcgalEngine::splitGeometry( const QgsLineString &splitLine, QVector<QgsGeometry> &newGeometries, bool topological, QgsPointSequence &topologyTestPoints, QString *errorMsg, bool skipIntersectionCheck ) const
{
  Q_UNUSED( splitLine );
  Q_UNUSED( newGeometries );
  Q_UNUSED( topological );
  Q_UNUSED( topologyTestPoints );
  Q_UNUSED( skipIntersectionCheck );
  Q_UNUSED( errorMsg );
  // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return MethodNotImplemented;
}

QgsAbstractGeometry *QgsSfcgalEngine::offsetCurve( double distance, int, Qgis::JoinStyle, double, QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *result = nullptr;
  result = sfcgal_geometry_offset_polygon( mSfcgalGeom.get(), distance ) ;

  if ( sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( result );
  return toSfcgalGeometry( sharedResult ).release();
}


QgsAbstractGeometry *QgsSfcgalEngine::triangulate( QString *errorMsg ) const
{
  sfcgal::errorHandler()->clearText();
  sfcgal_geometry_t *result = sfcgal_geometry_triangulate_2dz( mSfcgalGeom.get() );

  if ( !result || sfcgal::errorHandler()->hasFailed( errorMsg ) )
    return nullptr;

  sfcgal::shared_ptr sharedResult = sfcgal::make_shared_ptr( result );
  return toSfcgalGeometry( sharedResult ).release();
}

