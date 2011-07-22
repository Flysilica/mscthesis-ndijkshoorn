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

//char* g_instructions = "WASD: Move\r\nQE: Scale the mountains\r\nClick and drag the mouse to look around\r\nEsc: Quit\r\nF5: Toggle fullscreen\r\nF6: Toggle wireframe";

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: Default constructor
Parameters:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
terrain3d::terrain3d(short* map, UINT w, UINT h)
{
    m_pFramework = NULL;

	/**/
	elevation_map = map;
	elevation_map_w = w;
	elevation_map_h = h;
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
    if ( !pFramework->Initialize( "Camera Movement", hInstance, 640, 480, TRUE ) )
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
	m_pFramework->OnUpdateFrame();
	m_pFramework->OnRenderFrame();
}

void terrain3d::update_elevation_map(int* roi)
{
	//int roi[4] = {80, 120, 80, 120}; // x, y

	m_terrain.update(m_pFramework->m_pGraphics->GetDevice(), elevation_map, elevation_map_w, elevation_map_h, roi);
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
    m_camera.SetPosition( new D3DXVECTOR3( 0.0f, 5.0f, 20.0f ) );
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
    D3DXCreateSprite( pDevice, &m_pTextSprite );
    m_font.Initialize( pDevice, "Arial", 12 );

	m_terrain.Initialize( pDevice, elevation_map, elevation_map_w, elevation_map_h, "terrain.jpg" );
	//m_terrain.ScaleAbs( 0.5f, 0.01f, 0.5f );
	m_terrain.ScaleAbs( 0.5f, 0.05f, 0.5f );
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
    m_pTextSprite->OnResetDevice();
    m_font.OnResetDevice();

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
    m_pTextSprite->OnLostDevice();
    m_font.OnLostDevice();
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Summary: 
This callback function will be called immediately after the Direct3D device has been destroyed. 
Resources created in the OnCreateDevice callback should be released here, which generally includes 
all D3DPOOL_MANAGED resources.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void terrain3d::OnDestroyDevice()
{
    SAFE_RELEASE( m_pTextSprite );
    m_font.Release();
    m_terrain.Release();
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
    sprintf( m_fps, "%.2f fps", m_pFramework->GetFPS() );

    pDevice->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 255, 255, 255 ), 1.0f, 0 ); 
    pDevice->BeginScene();

    m_terrain.Render( pDevice );

    // Display framerate
    m_pTextSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
    m_font.Print( m_fps, 5, 5, D3DCOLOR_XRGB( 255, 0, 0 ), m_pTextSprite );
    m_pTextSprite->End();

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
    float cameraSpeed = 20.0f;
    if ( pMouseButtons[0] )
    {
        m_camera.Yaw( xDelta * elapsedTime * 1.8f);
        m_camera.Pitch( yDelta * elapsedTime * 1.8f );
    }
    if ( pPressedKeys[DIK_W] )
    {
        m_camera.MoveForward( cameraSpeed * elapsedTime );
    }
    if ( pPressedKeys[DIK_A] )
    {
        m_camera.Strafe( -cameraSpeed * elapsedTime );
    }
    if ( pPressedKeys[DIK_S] )
    {
        m_camera.MoveForward( -cameraSpeed * elapsedTime );
    }
    if ( pPressedKeys[DIK_D] )
    {
        m_camera.Strafe( cameraSpeed * elapsedTime );
    }
    if ( pPressedKeys[DIK_Q] )
    {
        m_terrain.ScaleRel( 0.0f, -0.5f * elapsedTime, 0.0f );
    }
    if ( pPressedKeys[DIK_E] )
    {
        m_terrain.ScaleRel( 0.0f , 0.5f * elapsedTime, 0.0f  );
    }
    if ( pPressedKeys[DIK_ESCAPE] )
    {
        m_pFramework->LockKey( DIK_ESCAPE );
        PostQuitMessage( 0 );
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
        }
    }
}