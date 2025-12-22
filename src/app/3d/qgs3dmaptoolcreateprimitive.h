/***************************************************************************
    qgs3dmaptoolcreateprimitive.h
    -------------------
    begin                : November 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DMAPTOOLCREATEPRIMITIVE_H
#define QGS3DMAPTOOLCREATEPRIMITIVE_H

#include "qgs3dmaptool.h"
#include "qgspoint.h"

#include <QObject>

class Qgs3DCreatePrimitiveDialog;
class QPoint;
class QgsRubberBand3D;

namespace Qt3DCore
{
  class QEntity;
}
namespace Qt3DExtras
{
  class QPhongMaterial;
}
namespace Qt3DRender
{
  class QMaterial;
  class QGeometryRenderer;
} //namespace Qt3DRender

/**
 * 3D maptool used to create 3D primitives from the 3D canvas
 */
class Qgs3DMapToolCreatePrimitive : public Qgs3DMapTool
{
    Q_OBJECT

  public:
    /// Primitive type to create
    enum PrimitiveType : int
    {
      Cylinder, //!< Cylinder
      Sphere,   //!< Sphere
      Cone,     //!< Cone
      Cube,     //!< Cube
      Box,      //!< Cube
      Torus,    //!< Torus
    };

    /**
     * Default constructor
     * @param canvas 3D canvas parent
     * @param type primitive type to create
     */
    Qgs3DMapToolCreatePrimitive( Qgs3DMapCanvas *canvas, PrimitiveType type );
    ~Qgs3DMapToolCreatePrimitive() override;

    void activate() override;
    void deactivate() override;

    QCursor cursor() const override;

    //! Reset and start new maptool
    void restart();
    //! Ends maptool
    void finish();

  private slots:
    void handleClick( const QPoint &screenPos );
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void keyReleaseEvent( QKeyEvent *event ) override;

  private:
    PrimitiveType mType;
    //! Dialog
    std::unique_ptr<Qgs3DCreatePrimitiveDialog> mDialog;
    std::unique_ptr<QgsRubberBand3D> mRubberBand;

    Qt3DRender::QGeometryRenderer *mCurrentMesh = nullptr;

    //! Indicates whether we've just done a right mouse click
    bool mDone = true;

    //! Check if mouse was moved between mousePress and mouseRelease
    bool mMouseHasMoved = false;
    QPoint mMouseClickPos;
    QPoint mMouseHoverPos;
    QVector<QgsPoint> mPointOnMap;
    int mNbMouseClick = 0;

    std::unique_ptr<Qt3DCore::QEntity> mPrimitiveLineEntity = nullptr;

    QgsPoint screenToMap( const QPoint &screenPos ) const;
    void updatePrimitive();
    void createPrimitive( bool enabledAttributeValuesDlg = true );
};

///@cond PRIVATE

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>

namespace QgsPrivate
{


  typedef Qt3DCore::QAttribute Qt3DQAttribute;
  typedef Qt3DCore::QGeometry Qt3DQGeometry;
  typedef Qt3DCore::QBuffer Qt3DQBuffer;

  class Qgs3DWiredMesh : public Qt3DRender::QGeometryRenderer
  {
      Q_OBJECT

    public:
      /**
     * \brief Default Qgs3DWiredMesh constructor
     */
      Qgs3DWiredMesh( Qt3DCore::QNode *parent = nullptr )
        : Qt3DRender::QGeometryRenderer( parent )
        , mPositionAttribute( new Qt3DQAttribute( this ) )
        , mVertexBuffer( new Qt3DQBuffer( this ) )
      {
        mPositionAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
        mPositionAttribute->setBuffer( mVertexBuffer );
        mPositionAttribute->setVertexBaseType( Qt3DQAttribute::Float );
        mPositionAttribute->setVertexSize( 3 );
        mPositionAttribute->setName( Qt3DQAttribute::defaultPositionAttributeName() );

        mGeom = new Qt3DQGeometry( this );
        mGeom->addAttribute( mPositionAttribute );

        setInstanceCount( 1 );
        setIndexOffset( 0 );
        setFirstInstance( 0 );
        setPrimitiveType( Qt3DRender::QGeometryRenderer::Lines );
        setGeometry( mGeom );
      }

      ~Qgs3DWiredMesh() override = default;

      /**
     * \brief add or replace mesh vertices coordinates
     */
      void setVertices( const QList<QVector3D> &vertices )
      {
        QByteArray vertexBufferData;
        vertexBufferData.resize( static_cast<int>( static_cast<long>( vertices.size() ) * 3 * sizeof( float ) ) );
        float *rawVertexArray = reinterpret_cast<float *>( vertexBufferData.data() );
        int idx = 0;
        for ( const QVector3D &v : std::as_const( vertices ) )
        {
          rawVertexArray[idx++] = v.x();
          rawVertexArray[idx++] = v.y();
          rawVertexArray[idx++] = v.z();
        }

        mVertexBuffer->setData( vertexBufferData );
        setVertexCount( vertices.count() );
      }

      /**
     * \brief add or replace mesh vertices coordinates from QgsAABB coordinates
     */
      //void setVertices( const QList<QgsAABB> &bboxes );

    private:
      Qt3DCore::QGeometry *mGeom = nullptr;
      Qt3DCore::QAttribute *mPositionAttribute = nullptr;
      Qt3DCore::QBuffer *mVertexBuffer = nullptr;
  };

  /// @endcond

} //namespace QgsPrivate

#endif // QGS3DMAPTOOLCREATEPRIMITIVE_H
