/***************************************************************************
  qgsframegraph.cpp
  --------------------------------------
  Date                 : August 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb underscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsframegraph.h"
#include "qgsdirectionallightsettings.h"
#include "qgspostprocessingentity.h"
#include "qgsdepthentity.h"
#include "qgs3dutils.h"
#include "qgsabstractrenderview.h"

#include "qgsambientocclusionrenderentity.h"
#include "qgsambientocclusionblurentity.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>

typedef Qt3DRender::QAttribute Qt3DQAttribute;
typedef Qt3DRender::QBuffer Qt3DQBuffer;
typedef Qt3DRender::QGeometry Qt3DQGeometry;
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QGeometry>

typedef Qt3DCore::QAttribute Qt3DQAttribute;
typedef Qt3DCore::QBuffer Qt3DQBuffer;
typedef Qt3DCore::QGeometry Qt3DQGeometry;
#endif

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QColorMask>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QBlendEquationArguments>
#include "qgsshadowrenderview.h"
#include "qgsforwardrenderview.h"
#include "qgsdepthrenderview.h"
#include "qgsdebugtexturerenderview.h"
#include "qgsdebugtextureentity.h"
#include "qgsambientocclusionrenderview.h"

const QString QgsFrameGraph::FORWARD_RENDERVIEW = "forward";
const QString QgsFrameGraph::SHADOW_RENDERVIEW = "shadow";
const QString QgsFrameGraph::AXIS3D_RENDERVIEW = "3daxis";
const QString QgsFrameGraph::DEPTH_RENDERVIEW = "depth";
const QString QgsFrameGraph::DEBUG_RENDERVIEW = "debug";
const QString QgsFrameGraph::AO_RENDERVIEW = "ambient_occlusion";

namespace
{

  QString formatNode( const Qt3DCore::QNode *n )
  {
    QString res = QString( QLatin1String( "%1{%2" ) )
                  .arg( QLatin1String( n->metaObject()->className() ) )
                  .arg( n->id().id() );
    if ( !n->objectName().isEmpty() )
      res += QString( QLatin1String( "/%1" ) ).arg( n->objectName() );
    res += "}";
    if ( !n->isEnabled() )
      res += QLatin1String( " [D]" );
    return res;
  }

  QString dumpEntity( const Qt3DCore::QEntity *n )
  {
    QString res = formatNode( n );
    const auto &components = n->components();
    if ( components.size() )
    {
      QStringList componentNames;
      for ( const auto &c : components )
      {
        componentNames << formatNode( c );
      }
      res += QString( QLatin1String( " [ %1 ]" ) ).arg( componentNames.join( QLatin1String( ", " ) ) );
    }

    return res;
  }

  QStringList dumpSG( const Qt3DCore::QNode *n, int level = 0 )
  {
    QStringList reply;
    const auto *entity = qobject_cast<const Qt3DCore::QEntity *>( n );
    if ( entity != nullptr )
    {
      QString res = dumpEntity( entity );
      reply += res.rightJustified( res.length() + level * 2, ' ' );
      level++;
    }

    const auto children = n->childNodes();
    for ( auto *child : children )
      reply += dumpSG( child, level );

    return reply;
  }

  QString dumpNode( const Qt3DRender::QFrameGraphNode *n )
  {
    QString res = formatNode( n );
    const auto *lf = qobject_cast<const Qt3DRender::QLayerFilter *>( n );
    if ( lf )
    {
      QStringList sl;
      const auto layers = lf->layers();
      for ( auto l : layers )
      {
        QString s = "{" + QString::number( l->id().id() );
        if ( !l->objectName().isEmpty() )
          s += QString( "/%1" ).arg( l->objectName() );
        sl += s + "}";
      }
      res += QString( " (%1: %2)" ).arg( QMetaEnum::fromType<Qt3DRender::QLayerFilter::FilterMode>().key( lf->filterMode() ), sl.join( QLatin1String( ", " ) ) );
    }
    auto cs = qobject_cast<const Qt3DRender::QCameraSelector *>( n );
    if ( cs )
      res += QString( " ({%1/%2})" ).arg( cs->camera()->id().id() ).arg( cs->camera()->objectName() );
    auto rs = qobject_cast<const Qt3DRender::QRenderTargetSelector *>( n );
    if ( rs && rs->target() )
    {
      QStringList sl;
      const auto outputs = rs->target()->outputs();
      for ( auto o : outputs )
      {
        sl += QString( "(%1: {%2/%3})" ).arg( QMetaEnum::fromType<Qt3DRender::QRenderTargetOutput::AttachmentPoint>().key( o->attachmentPoint() ), QString::number( o->texture()->id().id() ), o->texture()->objectName() );
      }
      res += QString( " " ) + sl.join( ", " );
    }
//    if (!n->isEnabled())
//        res += QLatin1String(" [D]");
    return res;
  }

  QStringList dumpFG( const Qt3DCore::QNode *n, int level = 0 )
  {
    QStringList reply;

    const Qt3DRender::QFrameGraphNode *fgNode = qobject_cast<const Qt3DRender::QFrameGraphNode *>( n );
    if ( fgNode )
    {
      QString res = dumpNode( fgNode );
      reply += res.rightJustified( res.length() + level * 2, ' ' );
    }

    const auto children = n->childNodes();
    const int inc = fgNode ? 1 : 0;
    for ( auto *child : children )
    {
      auto *childFGNode = qobject_cast<Qt3DCore::QNode *>( child );
      if ( childFGNode != nullptr )
        reply += dumpFG( childFGNode, level + inc );
    }

    return reply;
  }

} // End of namespace

QgsFrameGraph::~QgsFrameGraph()
{
  for ( QgsAbstractRenderView *rv : qAsConst( mRenderViewMap ) )
  {
    rv->deleteLater();
  }
}

void QgsFrameGraph::constructDebugTexturePass()
{
  QgsDebugTextureRenderView *depthRenderView = new QgsDebugTextureRenderView( this, mMainCamera );
  registerRenderView( depthRenderView, DEBUG_RENDERVIEW );
}

void QgsFrameGraph::constructForwardRenderPass()
{
  // This is where rendering of the 3D scene actually happens.
  // We define two forward passes: one for solid objects, followed by one for transparent objects.
  //
  //                                  |
  //                         +-----------------+
  //                         | QCameraSelector |  (using the main camera)
  //                         +-----------------+
  //                                  |
  //                         +-----------------+
  //                         |  QLayerFilter   |  (using mForwardRenderLayer)
  //                         +-----------------+
  //                                  |
  //                      +-----------------------+
  //                      | QRenderTargetSelector | (write mForwardColorTexture + mForwardDepthTexture)
  //                      +-----------------------+
  //                                  |
  //         +------------------------+---------------------+
  //         |                                              |
  //  +-----------------+    discard               +-----------------+    accept
  //  |  QLayerFilter   |  transparent             |  QLayerFilter   |  transparent
  //  +-----------------+    objects               +-----------------+    objects
  //         |                                              |
  //  +-----------------+  use depth test          +-----------------+   sort entities
  //  | QRenderStateSet |  cull back faces         |  QSortPolicy    |  back to front
  //  +-----------------+                          +-----------------+
  //         |                                              |
  //  +-----------------+              +--------------------+--------------------+
  //  | QFrustumCulling |              |                                         |
  //  +-----------------+     +-----------------+  use depth tests      +-----------------+  use depth tests
  //         |                | QRenderStateSet |  don't write depths   | QRenderStateSet |  write depths
  //         |                +-----------------+  write colors         +-----------------+  don't write colors
  //  +-----------------+                          use alpha blending                        don't use alpha blending
  //  |  QClearBuffers  |  color and depth         no culling                                no culling
  //  +-----------------+

  Qt3DRender::QTexture2D *forwardColorTexture = new Qt3DRender::QTexture2D;
  forwardColorTexture->setWidth( mSize.width() );
  forwardColorTexture->setHeight( mSize.height() );
  forwardColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  forwardColorTexture->setGenerateMipMaps( false );
  forwardColorTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardColorTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QTexture2D *forwardDepthTexture = new Qt3DRender::QTexture2D;
  forwardDepthTexture->setWidth( mSize.width() );
  forwardDepthTexture->setHeight( mSize.height() );
  forwardDepthTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  forwardDepthTexture->setGenerateMipMaps( false );
  forwardDepthTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardDepthTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QRenderTargetOutput *forwardRenderTargetDepthOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetDepthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  forwardRenderTargetDepthOutput->setTexture( forwardDepthTexture );
  forwardRenderTargetDepthOutput->setObjectName( "forwardRenderTargetDepthOutput" );
  Qt3DRender::QRenderTargetOutput *forwardRenderTargetColorOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetColorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );
  forwardRenderTargetColorOutput->setTexture( forwardColorTexture );
  forwardRenderTargetColorOutput->setObjectName( "forwardRenderTargetColorOutput" );

  QgsForwardRenderView *forwardRenderView = new QgsForwardRenderView( this, mMainCamera );
  forwardRenderView->setTargetOutputs( { forwardRenderTargetDepthOutput, forwardRenderTargetColorOutput } );
  registerRenderView( forwardRenderView, FORWARD_RENDERVIEW );
}

Qt3DRender::QLayer *QgsFrameGraph::transparentObjectLayer()
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    return forwardRenderView->transparentObjectLayer();
  return nullptr;
}

void QgsFrameGraph::constructShadowRenderPass()
{
  Qt3DRender::QTexture2D *shadowMapTexture = new Qt3DRender::QTexture2D;
  shadowMapTexture->setWidth( QgsFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setHeight( QgsFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  shadowMapTexture->setGenerateMipMaps( false );
  shadowMapTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  shadowMapTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );
  shadowMapTexture->setObjectName( "QgsShadowRenderView::DepthTarget" );

  Qt3DRender::QRenderTargetOutput *renderTargetOutput = new Qt3DRender::QRenderTargetOutput;
  renderTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  renderTargetOutput->setTexture( shadowMapTexture );

  QgsShadowRenderView *shadowRenderView = new QgsShadowRenderView( this );
  shadowRenderView->setTargetOutputs( { renderTargetOutput } );
  registerRenderView( shadowRenderView, SHADOW_RENDERVIEW );
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructPostprocessingPass()
{
  mPostProcessingCameraSelector = new Qt3DRender::QCameraSelector;
  mPostProcessingCameraSelector->setObjectName( "Postprocessing pass CameraSelector" );
  QgsShadowRenderView *shadowRenderView = dynamic_cast<QgsShadowRenderView *>( renderView( SHADOW_RENDERVIEW ) );
  mPostProcessingCameraSelector->setCamera( shadowRenderView->lightCamera() );

  mPostprocessPassLayerFilter = new Qt3DRender::QLayerFilter( mPostProcessingCameraSelector );

  mPostprocessClearBuffers = new Qt3DRender::QClearBuffers( mPostprocessPassLayerFilter );

  mRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mPostprocessClearBuffers );
  mRenderCaptureTargetSelector->setObjectName( "Postprocessing pass RenderTargetSelector" );

  Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget( mRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mRenderCaptureColorTexture = new Qt3DRender::QTexture2D( colorOutput );
  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  mRenderCaptureColorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureColorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureColorTexture->setObjectName( "PostProcessingPass::ColorTarget" );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mRenderCaptureColorTexture );
  renderTarget->addOutput( colorOutput );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  mRenderCaptureDepthTexture = new Qt3DRender::QTexture2D( depthOutput );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  mRenderCaptureDepthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  mRenderCaptureDepthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );
  mRenderCaptureDepthTexture->setObjectName( "PostProcessingPass::DepthTarget" );

  depthOutput->setTexture( mRenderCaptureDepthTexture );
  renderTarget->addOutput( depthOutput );

  mRenderCaptureTargetSelector->setTarget( renderTarget );

  mRenderCapture = new Qt3DRender::QRenderCapture( mRenderCaptureTargetSelector );

  Qt3DRender::QLayer *postProcessingLayer = new Qt3DRender::QLayer();
  mPostprocessingEntity = new QgsPostprocessingEntity( this, postProcessingLayer, mRootEntity );
  mPostprocessPassLayerFilter->addLayer( postProcessingLayer );
  mPostprocessingEntity->setObjectName( "PostProcessingPassEntity" );

  return mPostProcessingCameraSelector;
}

void QgsFrameGraph::constructAmbientOcclusionRenderPass()
{
//  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
//  cameraselector->setObjectName( "AmbientOcclusion render pass CameraSelector" );
//  cameraSelector->setCamera( mMainCamera );

//  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

//  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
//  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
//  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
//  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

//  renderStateSet->addRenderState( depthRenderDepthTest );
//  renderStateSet->addRenderState( depthRenderCullFace );

//  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( renderStateSet );

//  Qt3DRender::QRenderTargetSelector *targetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );
//  Qt3DRender::QRenderTarget *colorRenderTarget = new Qt3DRender::QRenderTarget( targetSelector );

//  // The lifetime of the objects created here is managed
//  // automatically, as they become children of this object.

//  // Create a render target output for rendering color.
//  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput( colorRenderTarget );
//  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

//  // Create a texture to render into.
//  mAmbientOcclusionRenderTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
//  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
//  mAmbientOcclusionRenderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
//  mAmbientOcclusionRenderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
//  mAmbientOcclusionRenderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

//  // Hook the texture up to our output, and the output up to this object.
//  colorTargetOutput->setTexture( mAmbientOcclusionRenderTexture );
//  colorRenderTarget->addOutput( colorTargetOutput );

//  targetSelector->setTarget( colorRenderTarget );

//  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
//  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
//  mAmbientOcclusionRenderEntity = new QgsAmbientOcclusionRenderEntity( forwardDepthTexture, mMainCamera, mRootEntity );
//  layerFilter->addLayer( mAmbientOcclusionRenderEntity->layer() );

  // Create a texture to render into.
  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput;
  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  Qt3DRender::QTexture2D *renderTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
  renderTexture->setSize( mSize.width(), mSize.height() );
  renderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  renderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  renderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  renderTexture->setObjectName( "QgsAmbientOcclusionRenderView::ColorTarget" );
  colorTargetOutput->setTexture( renderTexture );

  QgsAmbientOcclusionRenderView *aorv = new QgsAmbientOcclusionRenderView( this, mMainCamera );
  aorv->setTargetOutputs( { colorTargetOutput } );
  registerRenderView( aorv, AO_RENDERVIEW );
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructAmbientOcclusionBlurPass()
{
  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
  cameraSelector->setObjectName( "AmbientOcclusion blur pass CameraSelector" );
  cameraSelector->setCamera( mMainCamera );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  renderStateSet->addRenderState( depthRenderDepthTest );
  renderStateSet->addRenderState( depthRenderCullFace );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( renderStateSet );

  Qt3DRender::QRenderTargetSelector *targetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( targetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionBlurTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionBlurTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionBlurTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorTargetOutput->setTexture( mAmbientOcclusionBlurTexture );
  depthRenderTarget->addOutput( colorTargetOutput );

  targetSelector->setTarget( depthRenderTarget );

  Qt3DRender::QLayer *ambientOcclusionBlurlayer = new Qt3DRender::QLayer();
  Qt3DRender::QTexture2D *renderTexture = renderView( QgsFrameGraph::AO_RENDERVIEW )->outputTexture( Qt3DRender::QRenderTargetOutput::Color0 );
  Q_ASSERT( renderTexture != nullptr );
  mAmbientOcclusionBlurEntity = new QgsAmbientOcclusionBlurEntity( renderTexture, ambientOcclusionBlurlayer, mRootEntity );
  layerFilter->addLayer( ambientOcclusionBlurlayer );

  return cameraSelector;
}


Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructRubberBandsPass()
{
  mRubberBandsCameraSelector = new Qt3DRender::QCameraSelector;
  mRubberBandsCameraSelector->setObjectName( "RubberBands Pass CameraSelector" );
  mRubberBandsCameraSelector->setCamera( mMainCamera );

  mRubberBandsLayerFilter = new Qt3DRender::QLayerFilter( mRubberBandsCameraSelector );
  mRubberBandsLayerFilter->addLayer( mRubberBandsLayer );

  mRubberBandsStateSet = new Qt3DRender::QRenderStateSet( mRubberBandsLayerFilter );
  Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
  depthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  mRubberBandsStateSet->addRenderState( depthTest );

  // Here we attach our drawings to the render target also used by forward pass.
  // This is kind of okay, but as a result, post-processing effects get applied
  // to rubber bands too. Ideally we would want them on top of everything.
  mRubberBandsRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( mRubberBandsStateSet );
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  mRubberBandsRenderTargetSelector->setTarget( forwardRenderView->renderTargetSelector()->target() );

  return mRubberBandsCameraSelector;
}



void QgsFrameGraph::constructDepthRenderPass()
{
  // depth buffer render to copy pass
  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput;
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  Qt3DRender::QTexture2D *colorTexture = new Qt3DRender::QTexture2D( colorOutput );
  colorTexture->setSize( mSize.width(), mSize.height() );
  colorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  colorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  colorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  colorTexture->setObjectName( "QgsDepthRenderView::ColorTarget" );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( colorTexture );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput;

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DRender::QTexture2D *depthTexture = new Qt3DRender::QTexture2D( depthOutput );
  depthTexture->setSize( mSize.width(), mSize.height() );
  depthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  depthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  depthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );
  depthTexture->setObjectName( "QgsDepthRenderView::DepthTarget" );

  depthOutput->setTexture( depthTexture );

  QgsDepthRenderView *depthRenderView = new QgsDepthRenderView( this, mMainCamera );
  depthRenderView->setTargetOutputs( { colorOutput, depthOutput } );
  registerRenderView( depthRenderView, DEPTH_RENDERVIEW );

  // entity used to draw the depth texture and convert it to rgb image
  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DCore::QEntity *quad = new QgsDepthEntity( forwardDepthTexture, depthRenderView->layerToFilter(), mRootEntity );
  quad->setParent( mRootEntity );
}

Qt3DRender::QRenderCapture *QgsFrameGraph::depthRenderCapture()
{
  QgsDepthRenderView *depthRenderView = dynamic_cast<QgsDepthRenderView *>( renderView( DEPTH_RENDERVIEW ) );
  if ( depthRenderView )
    return depthRenderView->renderCapture();

  return nullptr;
}

QgsFrameGraph::QgsFrameGraph( QSurface *surface, QSize s, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root )
  : Qt3DCore::QEntity( root )
  , mSize( s )
{

  // general overview of how the frame graph looks:
  //
  //  +------------------------+    using window or
  //  | QRenderSurfaceSelector |   offscreen surface
  //  +------------------------+
  //             |
  //  +-----------+
  //  | QViewport | (0,0,1,1)
  //  +-----------+
  //             |
  //     +--------------------------+-------------------+-----------------+
  //     |                          |                   |                 |
  // +--------------------+ +--------------+ +-----------------+ +-----------------+
  // | two forward passes | | shadows pass | |  depth buffer   | | post-processing |
  // |  (solid objects    | |              | | processing pass | |    passes       |
  // |  and transparent)  | +--------------+ +-----------------+ +-----------------+
  // +--------------------+
  //
  // Notes:
  // - depth buffer processing pass is used whenever we need depth map information
  //   (for camera navigation) and it converts depth texture to a color texture
  //   so that we can capture it with QRenderCapture - currently it is unable
  //   to capture depth buffer, only colors (see QTBUG-65155)
  // - there are multiple post-processing passes that take rendered output
  //   of the scene, optionally apply effects (add shadows, ambient occlusion,
  //   eye dome lighting) and finally output to the given surface
  // - there may be also two more passes when 3D axis is shown - see Qgs3DAxis

  mRootEntity = root;
  mMainCamera = mainCamera;

  mRubberBandsLayer = new Qt3DRender::QLayer;
  mRubberBandsLayer->setObjectName( "mRubberBandsLayer" );
  mRubberBandsLayer->setRecursive( true );

  mRenderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector;

  QObject *surfaceObj = dynamic_cast< QObject *  >( surface );
  Q_ASSERT( surfaceObj );

  mRenderSurfaceSelector->setSurface( surfaceObj );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  mMainViewPort = new Qt3DRender::QViewport( mRenderSurfaceSelector );
  mMainViewPort->setNormalizedRect( QRectF( 0.0f, 0.0f, 1.0f, 1.0f ) );

  // Forward render
  constructForwardRenderPass();

  // rubber bands (they should be always on top)
  Qt3DRender::QFrameGraphNode *rubberBandsPass = constructRubberBandsPass();
  rubberBandsPass->setObjectName( "rubberBandsPass" );
  rubberBandsPass->setParent( mMainViewPort );

  // depth buffer processing
  constructDepthRenderPass();

  // Ambient occlusion factor render pass
  constructAmbientOcclusionRenderPass();

  // TODO: TEMP DISABLE
  Qt3DRender::QFrameGraphNode *ambientOcclusionBlurPass = constructAmbientOcclusionBlurPass();
  ambientOcclusionBlurPass->setParent( mMainViewPort );
  // mAmbientOcclusionBlurTexture = renderView( QgsFrameGraph::AO_RENDERVIEW )->outputTexture( Qt3DRender::QRenderTargetOutput::Color0 );

  // shadow rendering pass
  constructShadowRenderPass();

  // post process
  Qt3DRender::QFrameGraphNode *postprocessingPass = constructPostprocessingPass();
  postprocessingPass->setParent( mMainViewPort );
  postprocessingPass->setObjectName( "PostProcessingPass" );

  mRubberBandsRootEntity = new Qt3DCore::QEntity( mRootEntity );
  mRubberBandsRootEntity->setObjectName( "mRubberBandsRootEntity" );
  mRubberBandsRootEntity->addComponent( mRubberBandsLayer );

  // textures preview pass
  constructDebugTexturePass();
}

void QgsFrameGraph::unregisterRenderView( const QString &name )
{
  QgsAbstractRenderView *renderView = mRenderViewMap [name];
  if ( renderView )
  {
    renderView->topGraphNode()->setParent( ( QNode * )nullptr );
    mRenderViewMap.remove( name );
  }
}

bool QgsFrameGraph::registerRenderView( QgsAbstractRenderView *renderView, const QString &name )
{
  bool out;
  if ( mRenderViewMap [name] == nullptr )
  {
    mRenderViewMap [name] = renderView;
    renderView->topGraphNode()->setParent( mMainViewPort );
    out = true;
  }
  else
    out = false;

  return out;
}

void QgsFrameGraph::setEnableRenderView( const QString &name, bool enable )
{
  if ( mRenderViewMap [name] )
  {
    mRenderViewMap [name]->enableSubTree( enable );
  }
}

QgsAbstractRenderView *QgsFrameGraph::renderView( const QString &name )
{
  return mRenderViewMap [name];
}

Qt3DRender::QLayer *QgsFrameGraph::filterLayer( const QString &name )
{
  return mRenderViewMap [name]->layerToFilter();
}

bool QgsFrameGraph::isRenderViewEnabled( const QString &name )
{
  return mRenderViewMap [name] != nullptr && mRenderViewMap [name]->isSubTreeEnabled();
}

void QgsFrameGraph::dump()
{
  auto fg = dumpFG( mRenderSurfaceSelector );
  auto sg = dumpSG( mRootEntity );

  qDebug() << "FrameGraph:\n" << qPrintable( fg.join( "\n" ) );
  qDebug() << "SceneGraph:\n" << qPrintable( sg.join( "\n" ) );
}

void QgsFrameGraph::setClearColor( const QColor &clearColor )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->setClearColor( clearColor );
}

//void QgsFrameGraph::setAmbientOcclusionEnabled( bool enabled )
//{
//  mAmbientOcclusionRenderEntity->setEnabled( enabled );
//  mPostprocessingEntity->setAmbientOcclusionEnabled( enabled );
//}

//void QgsFrameGraph::setAmbientOcclusionIntensity( float intensity )
//{
//  mAmbientOcclusionRenderEntity->setIntensity( intensity );
//}

//void QgsFrameGraph::setAmbientOcclusionRadius( float radius )
//{
//  mAmbientOcclusionRenderEntity->setRadius( radius );
//}

//void QgsFrameGraph::setAmbientOcclusionThreshold( float threshold )
//{
//  mAmbientOcclusionRenderEntity->setThreshold( threshold );
//}

void QgsFrameGraph::setFrustumCullingEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableFrustumCulling( enabled );
}

void QgsFrameGraph::setupEyeDomeLighting( bool enabled, double strength, int distance )
{
  mPostprocessingEntity->setEyeDomeLightingEnabled( enabled );
  mPostprocessingEntity->setEyeDomeLightingStrength( strength );
  mPostprocessingEntity->setEyeDomeLightingDistance( distance );
}

void QgsFrameGraph::setSize( QSize s )
{
  mSize = s;

  for ( QgsAbstractRenderView *renderView : qAsConst( mRenderViewMap ) )
  {
    renderView->updateTargetOutputSize( mSize.width(), mSize.height() );
  }

  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  // TODO: TEMP DISABLE
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
}

void QgsFrameGraph::setRenderCaptureEnabled( bool enabled )
{
  if ( enabled == mRenderCaptureEnabled )
    return;
  mRenderCaptureEnabled = enabled;
  mRenderCaptureTargetSelector->setEnabled( mRenderCaptureEnabled );
}

void QgsFrameGraph::setDebugOverlayEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableDebugOverlay( enabled );
}
