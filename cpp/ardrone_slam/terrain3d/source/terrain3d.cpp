/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

Title : WinMain.cpp
Author : Chad Vernon
URL : http://www.c-unit.com

Description : Camera

Created :  08/26/2005
Modified : 12/06/2005

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "..\include\stdafx.h"
#include "..\include\terrain3d.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Default constructor
Parameters:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
terrain3d::terrain3d(short* map, UINT w, UINT h, byte* texture, float* arrow, float* waypoint)
{
    m_pFramework = NULL;

	/**/
	updated				= true;
	elevation_map		= map;
	elevation_map_w		= w;
	elevation_map_h		= h;
	this->texture		= texture;
	this->arrow			= arrow;
	this->waypoint		= waypoint;
	/**/

	HINSTANCE hInstance = GetModuleHandle(NULL);

	terrain3d* pApplication = this;
    CFramework* pFramework = new CFramework( (CBaseApp*)pApplication );

    pApplication->SetFramework( pFramework );

    // Initialize any application resources
    if ( !pApplication->Initialize() )
    {
        return;
    }

    // Initialize the Framework
    if ( !pFramework->Initialize( "3D Terrain Map", hInstance, 640, 480, TRUE ) )
    {
        return;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Clean up resources
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::Release()
{
    SAFE_RELEASE( m_pFramework );
}
terrain3d::~terrain3d()
{
    SAFE_RELEASE( m_pFramework );
}

void terrain3d::render()
{
	updated = false;
	//m_pFramework->OnUpdateFrame();
	m_pFramework->OnRenderFrame();
}

void terrain3d::update_elevation_map(int* roi)
{
	m_terrain.update_elevation_map(m_pFramework->m_pGraphics->GetDevice(), elevation_map, elevation_map_w, elevation_map_h, roi);
	updated = true;
}

void terrain3d::update_texture(int *roi)
{
	m_terrain.update_texture(texture, roi);
	updated = true;
}

bool terrain3d::requires_render()
{
	return updated;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Sets the CFramework instnace of the application.
Parameters:
[in] pFramework - Pointer to a CFramework instance
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::SetFramework( CFramework* pFramework )
{
    m_pFramework = pFramework;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Initialize application-specific resources and states here.
Returns: TRUE on success, FALSE on failure
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
BOOL terrain3d::Initialize()
{
    m_camera.SetMaxVelocity( 100.0f );
    m_camera.SetPosition( new D3DXVECTOR3( 0.0f, 10.0f, -20.0f ) );
    m_camera.SetLookAt( new D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    m_camera.Update();
    return TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: 
This callback function will be called immediately after the Direct3D device has been created. This is 
the best location to create D3DPOOL_MANAGED resources. Resources created here should be released in 
the OnDestroyDevice callback. 
Parameters:
[in] pDevice - Pointer to a DIRECT3DDEVICE9 instance
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnCreateDevice( LPDIRECT3DDEVICE9 pDevice )
{
	m_terrain.Initialize( pDevice, elevation_map, elevation_map_w, elevation_map_h, texture );
	m_terrain.ScaleAbs( 0.5f, /*0.02f*/ 0.01f, 0.5f ); // 50mm x 1mm x 50mm

	// NICK
	m_arrow.Initialize( pDevice, arrow );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: 
This callback function will be called immediately after the Direct3D device has been created. This is 
the best location to create D3DPOOL_DEFAULT resources. Resources created here should be released in 
the OnLostDevice callback. 
Parameters:
[in] pDevice - Pointer to a DIRECT3DDEVICE9 instance
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnResetDevice( LPDIRECT3DDEVICE9 pDevice )
{
    // Set projection
    m_camera.SetAspectRatio( (float)m_pFramework->GetWidth() / (float)m_pFramework->GetHeight() );
    pDevice->SetTransform( D3DTS_PROJECTION, m_camera.GetProjectionMatrix() );

    // Set render states
    pDevice->SetRenderState( D3DRS_FILLMODE, m_pFramework->GetFillMode() );      
    pDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );  
    pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: 
This callback function will be called immediately after the Direct3D device has entered a lost state 
and before IDirect3DDevice9::Reset is called. Resources created in the OnResetDevice callback should 
be released here, which generally includes all D3DPOOL_DEFAULT resources.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnLostDevice()
{
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: 
This callback function will be called immediately after the Direct3D device has been destroyed. 
Resources created in the OnCreateDevice callback should be released here, which generally includes 
all D3DPOOL_MANAGED resources.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnDestroyDevice()
{
    m_terrain.Release();
	m_arrow.Release();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Updates the current frame.
Parameters:
[in] pDevice - Pointer to a DIRECT3DDEVICE9 instance
[in] elapsedTime - Time elapsed since last frame
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnUpdateFrame( LPDIRECT3DDEVICE9 pDevice, float elapsedTime )
{
    m_camera.Update();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Renders the current frame.
Parameters:
[in] pDevice - Pointer to a DIRECT3DDEVICE9 instance
[in] elapsedTime - Time elapsed since last frame
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnRenderFrame( LPDIRECT3DDEVICE9 pDevice, float elapsedTime )
{
    pDevice->SetTransform( D3DTS_VIEW, m_camera.GetViewMatrix() );

    pDevice->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 135, 206, 250 ), 1.0f, 0 ); 
    pDevice->BeginScene();

	m_terrain.Render( pDevice );
	m_arrow.Render( pDevice );

    pDevice->EndScene();
    pDevice->Present( 0, 0, 0, 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Responds to key presses
Parameters:
[in] xDelta - Change in mouse x-axis since last frame
[in] yDelta - Change in mouse y-axis since last frame
[in] zDelta - Change in mouse z-axis since last frame
[in] pMouseButtons - Mouse button states
[in] pPressedKeys - Keyboard keys that are pressed and not locked
[in] elapsedTime - Time elapsed since last frame
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::ProcessInput( long xDelta, long yDelta, long zDelta, BOOL* pMouseButtons, BOOL* pPressedKeys, float elapsedTime )
{
    float cameraSpeed = 30.0f;

	if ( pMouseButtons[1] )
	{
			D3DVIEWPORT9 viewport;

			m_pFramework->m_pGraphics->GetDevice()->GetViewport(&viewport);

			D3DXVECTOR3 inP1( (float) m_pFramework->m_mouse.GetX(), (float) m_pFramework->m_mouse.GetY(), 0.1f);
			D3DXVECTOR3 outP1;
			D3DXVECTOR3 outP2;

			D3DXMATRIX viewMatrix;
			m_pFramework->m_pGraphics->GetDevice()->GetTransform( D3DTS_VIEW, &viewMatrix);
			D3DXMATRIX projMatrix;
			m_pFramework->m_pGraphics->GetDevice()->GetTransform( D3DTS_PROJECTION, &projMatrix);
			D3DXMATRIX worldMatrix;
			m_pFramework->m_pGraphics->GetDevice()->GetTransform( D3DTS_WORLD, &worldMatrix);

			outP1 = *m_camera.GetPosition();

			inP1.z = viewport.MaxZ;

			D3DXVec3Unproject(&outP2, &inP1, &viewport, /*&m_camera.m_projection*/ /*m_camera.GetProjectionMatrix()*/ &projMatrix, /*&m_camera.m_view*//*m_camera.GetViewMatrix()*/&viewMatrix, /*&worldMatrix*/ /*m_terrain.GetTransform()*/&worldMatrix);
			//printf("DEBUG: %f, %f, %f\n", outP2.x, outP2.y, outP2.z);

			D3DXVECTOR3 intersection;
			D3DXPLANE p(0.0f, 1.0f, 0.0f, 0.0f);
			D3DXPlaneIntersectLine(&intersection, &p, &outP1, &outP2);

			//printf("DEBUG: %f, %f\n", intersection.x * 100.0f, intersection.z * 100.0f);
			waypoint[0] = intersection.z * 100.0f;
			waypoint[1] = intersection.x * 100.0f;
	}

    if ( pMouseButtons[0] )
    {
        m_camera.Yaw( xDelta * elapsedTime * 0.1f);
        m_camera.Pitch( yDelta * elapsedTime * 0.1f );
		updated = true;
    }
    if ( pPressedKeys[DIK_I] )
    {
        m_camera.MoveForward( cameraSpeed * elapsedTime );
		updated = true;
    }
    if ( pPressedKeys[DIK_J] )
    {
        m_camera.Strafe( -cameraSpeed * elapsedTime );
		updated = true;
    }
    if ( pPressedKeys[DIK_K] )
    {
        m_camera.MoveForward( -cameraSpeed * elapsedTime );
		updated = true;
    }
    if ( pPressedKeys[DIK_L] )
    {
        m_camera.Strafe( cameraSpeed * elapsedTime );
		updated = true;
    }
    if ( pPressedKeys[DIK_Q] )
    {
        m_terrain.ScaleRel( 0.0f, -0.1f * elapsedTime, 0.0f );
		updated = true;
    }
    if ( pPressedKeys[DIK_E] )
    {
        m_terrain.ScaleRel( 0.0f , 0.1f * elapsedTime, 0.0f  );
		updated = true;
    }
    if ( pPressedKeys[DIK_F5] )
    {
        m_pFramework->LockKey( DIK_F5 );
        if ( m_pFramework != NULL )
        {
            m_pFramework->ToggleFullscreen();
        }
    }
    if ( pPressedKeys[DIK_F6] )
    {
        m_pFramework->LockKey( DIK_F6 );
        if ( m_pFramework != NULL )
        {
            m_pFramework->ToggleWireframe();
			updated = true;
        }
    }
}