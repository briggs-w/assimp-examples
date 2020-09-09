#include "UVTexturedDemo.h"
#include <Color.h>

#include <iostream>

namespace sess
{

UVTexturedDemo::UVTexturedDemo(HINSTANCE appHandle)
	: DemoApp(appHandle, L"Demo - Drawing with Materials Only")
	, materialOnlyShader_()
	, texturedShader_()
	, camera_(Vec3(0.f, 2.2f, 0.f), Vec3(0.f, 2.2f, 1.f), Vec3::UnitY)
	, projMatrix_(PerspectiveLH(Radians(80.f), (windowSize_.right - windowSize_.left) / (float)(windowSize_.bottom - windowSize_.top), 0.1f, 100.f))
	, debugIcosphere_(nullptr)
	, roadModel_(nullptr)
	, manModel_(nullptr)
	, inputState_({ /* Initialize to all false */ })
{}

UVTexturedDemo::~UVTexturedDemo()
{}

//
// Overrides
//
bool UVTexturedDemo::InitializeApp()
{
	std::future<bool> shaderLoaded = materialOnlyShader_.Initialize(device_);
	std::future<bool> textureShaderLoaded = texturedShader_.Initialize(device_);

	debugIcosphere_ = std::make_shared<DebugMaterialIcosphere>
		(
			device_,
			MaterialOnlyShader::Material(Color::Palette::Red.withAlpha(0.8f), Color::Palette::Red, Color::Palette::Red.clampAndScale(0.9f)),
			Vec3(0.f, 1.8f, 6.f),
			Vec3::Ones,
			1.1f
			);

	Transform roadTransform(Vec3::Zero, Quaternion(Vec3::UnitY, Radians(-90.f)) * Quaternion(Vec3::UnitX, Radians(-90.f)), Vec3::Ones);
	roadModel_ = AssimpRoadModel::LoadFromFile("../assets/road.fbx", device_, roadTransform);
	if (!roadModel_)
	{
		std::cerr << "Failed to load road model, failing initialization" << std::endl;
		return 0;
	}

	Transform manTransform
	(
		Vec3(0.f, 1.21f, 3.f),
		Quaternion(Vec3::UnitY, Radians(180.f)) * Quaternion(Vec3::UnitX, Radians(-90.f)),
		Vec3(0.55, 0.55, 0.55)
	);
	manModel_ = AssimpManModel::LoadFromFile("../assets/simpleMan2.6.fbx", "../assets/man-skin.png", device_, context_, manTransform);
	if (!manModel_)
	{
		std::cerr << "Failed to load man model, failing initialization" << std::endl;
		return 0;
	}

	if (shaderLoaded.get() == false)
	{
		std::cerr << "Failed to load material only shader in UV demo app" << std::endl;
		return false;
	}

	if (textureShaderLoaded.get() == false)
	{
		std::cerr << "Failed to load texture shader in UV demo app" << std::endl;
		return false;
	}

	MaterialOnlyShader::DirectionalLight sun
	(
		Vec3(2.f, -1.6f, 3.f).Normal(),
		Color::Palette::PureWhite,
		Color::Palette::PureWhite,
		Color::Palette::PureWhite.clampAndScale(0.15f)
	);
	materialOnlyShader_.SetSunLight(sun);

	return true;
}

bool UVTexturedDemo::Update(float dt)
{
	const static float ROTATE_SPEED = 1.8f;
	const static float MOVE_SPEED = 10.f;

	if (inputState_.W_Pressed && !inputState_.S_Pressed)
	{
		camera_.MoveForward(dt / 1000.f * MOVE_SPEED);
	}
	if (inputState_.S_Pressed && !inputState_.W_Pressed)
	{
		camera_.MoveForward(dt / 1000.f * -MOVE_SPEED);
	}
	if (inputState_.A_Pressed && !inputState_.D_Pressed)
	{
		camera_.MoveRight(dt / 1000.f * -MOVE_SPEED);
	}
	if (inputState_.D_Pressed && !inputState_.A_Pressed)
	{
		camera_.MoveRight(dt / 1000.f * MOVE_SPEED);
	}

	if (inputState_.Left_Pressed && !inputState_.Right_Pressed)
	{
		camera_.RotateRight(dt / 1000.f * -ROTATE_SPEED);
	}
	if (inputState_.Right_Pressed && !inputState_.Left_Pressed)
	{
		camera_.RotateRight(dt / 1000.f * ROTATE_SPEED);
	}

	debugIcosphere_->Update(dt);
	roadModel_->Update(dt);

	return true;
}

bool UVTexturedDemo::Render()
{
	float colors[4];
	Color::Palette::Indigo.packAsFloatArray(colors);
	context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
	context_->ClearRenderTargetView(renderTargetView_.Get(), colors);
	context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0x00);
	context_->RSSetViewports(1, &viewport_);

	materialOnlyShader_.SetCameraPosition(camera_.GetPosition());
	materialOnlyShader_.SetViewTransform(camera_.GetViewMatrix());
	materialOnlyShader_.SetProjectionTransform(projMatrix_);

	texturedShader_.SetCameraPosition(camera_.GetPosition());
	texturedShader_.SetViewTransform(camera_.GetViewMatrix());
	texturedShader_.SetProjectionTransform(projMatrix_);

	debugIcosphere_->Render(context_, &materialOnlyShader_);
	roadModel_->Render(context_, &materialOnlyShader_);
	manModel_->Render(context_, &texturedShader_);

	swapChain_->Present(1, 0x00);

	return true;
}

LRESULT UVTexturedDemo::HandleWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Handle key presses
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W': inputState_.W_Pressed = true; break;
		case 'A': inputState_.A_Pressed = true; break;
		case 'D': inputState_.D_Pressed = true; break;
		case 'S': inputState_.S_Pressed = true; break;
		case VK_LEFT: inputState_.Left_Pressed = true; break;
		case VK_RIGHT: inputState_.Right_Pressed = true; break;
		case VK_ESCAPE: PostQuitMessage(0); break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'W': inputState_.W_Pressed = false; break;
		case 'A': inputState_.A_Pressed = false; break;
		case 'D': inputState_.D_Pressed = false; break;
		case 'S': inputState_.S_Pressed = false; break;
		case VK_LEFT: inputState_.Left_Pressed = false; break;
		case VK_RIGHT: inputState_.Right_Pressed = false; break;
		}
		break;
	}

	return DemoApp::HandleWin32Message(hWnd, msg, wParam, lParam);
}

};