/***************************************************************************
  qgsabstractrenderview.cpp
  --------------------------------------
  Date                 : June 2024
  Copyright            : (C) 2024 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsabstractrenderview.h"
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/qsubtreeenabler.h>
#include <Qt3DRender/QRenderTarget>
#include <Qt3DRender/QRenderTargetSelector>

QgsAbstractRenderView::QgsAbstractRenderView( QObject *parent, const QString &viewName )
  : QObject( parent ), mViewName( viewName )
{
  std::tie( mRoot, mRendererEnabler ) = createSubtreeEnabler();
}

void QgsAbstractRenderView::setTargetOutputs( const QList<Qt3DRender::QRenderTargetOutput *> &targetOutputList )
{
  mTargetOutputs = targetOutputList;
  onTargetOutputUpdate();
}

void QgsAbstractRenderView::updateTargetOutputSize( int width, int height )
{
  for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
    targetOutput->texture()->setSize( width, height );
}

Qt3DRender::QTexture2D *QgsAbstractRenderView::outputTexture( Qt3DRender::QRenderTargetOutput::AttachmentPoint attachment )
{
  for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
    if ( targetOutput->attachmentPoint() == attachment )
      return ( Qt3DRender::QTexture2D * )targetOutput->texture();

  return nullptr;
}

std::pair<Qt3DRender::QFrameGraphNode *, Qt3DRender::QSubtreeEnabler *> QgsAbstractRenderView::createSubtreeEnabler( Qt3DRender::QFrameGraphNode *parent )
{
  // in order to avoid a render pass on the render view, we add a NoDraw node
  // which is disabled when the enabler is enabled, and vice versa
  using namespace Qt3DRender;
  auto noDraw = new QNoDraw( parent );
  noDraw->setEnabled( false );
  auto enabler = new QSubtreeEnabler( noDraw );
  enabler->setEnablement( QSubtreeEnabler::Persistent );
  connect( enabler, &QSubtreeEnabler::enabledChanged, noDraw, [noDraw]( bool enable ) { noDraw->setEnabled( !enable ); } );
  return std::make_pair( noDraw, enabler );
}

void QgsAbstractRenderView::onTargetOutputUpdate()
{
  if ( mRenderTargetSelector )
  {
    if ( ! mTargetOutputs.isEmpty() )
    {
      Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget;
      renderTarget->setObjectName( mViewName + "::Target" );

      for ( Qt3DRender::QRenderTargetOutput *targetOutput : qAsConst( mTargetOutputs ) )
        renderTarget->addOutput( targetOutput );

      mRenderTargetSelector->setTarget( renderTarget );
    }
    else
    {
      mRenderTargetSelector->setTarget( nullptr );
    }
  }
}

Qt3DRender::QLayer *QgsAbstractRenderView::layerToFilter()
{
  return mLayer;
}

Qt3DRender::QViewport *QgsAbstractRenderView::viewport()
{
  return nullptr;
}

Qt3DRender::QFrameGraphNode *QgsAbstractRenderView::topGraphNode()
{
  return mRoot;
}

void QgsAbstractRenderView::enableSubTree( bool enable )
{
  mRoot->setEnabled( ! enable );
  mRendererEnabler->setEnabled( enable );
}

bool QgsAbstractRenderView::isSubTreeEnabled()
{
  return ! mRoot->isEnabled() && mRendererEnabler->isEnabled();
}
