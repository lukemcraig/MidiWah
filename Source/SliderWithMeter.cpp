/*
  ==============================================================================

    SliderWithMeter.cpp
    Created: 15 Oct 2019 7:29:54pm
    Author:  Luke

  ==============================================================================
*/

#include "SliderWithMeter.h"

SliderWithMeter::SliderWithMeter(OpenGLContext& oglc, ParameterHelper& ph) : openGLContext(oglc), parameterHelper(ph)
{
}

SliderWithMeter::~SliderWithMeter()
{
}

void SliderWithMeter::initialiseOpenGL()
{
    createShaders();
}

void SliderWithMeter::shutdown()
{
    shaderProgram.reset();

    quad.reset();
    attributes.reset();
    uniforms.reset();
}

void SliderWithMeter::renderScene()
{
    //render scene
    auto desktopScale = (float)openGLContext.getRenderingScale();
    auto width = roundToInt(desktopScale * getWidth());
    auto height = roundToInt(desktopScale * getHeight());

    auto x = getRight() * desktopScale - width;
    auto y = (getParentHeight() - getBottom()) * desktopScale;
    glViewport(x, y, width, height);

    shaderProgram->use();

    if (uniforms->iResolution != nullptr)
    {
        uniforms->iResolution->set(width, height);
    }

    if (uniforms->iViewport != nullptr)
    {
        uniforms->iViewport->set(x, y);
    }

    if (uniforms->iLevel != nullptr)
    {
        uniforms->iLevel->set(parameterHelper.getCurrentMix(0));
    }

    quad->draw(openGLContext, *attributes);
}

void SliderWithMeter::render()
{
    jassert(OpenGLHelpers::isContextActive());

    renderScene();

    // Reset the element buffers so child Components draw correctly
    openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
    openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SliderWithMeter::createShaders()
{
    vertexShader =
        "attribute vec4 position;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position.xy*5.0,0.0,1.0);\n"
        "}\n";

    fragmentShader =
        "uniform vec2 iResolution;\n"
        "uniform vec2 iViewport;\n"
        "uniform float iLevel;\n"

        "#define quietColor vec3(0.0, 0.0, 0.0)\n"
        "#define loudColor vec3(1.0, 1.0, 1.0)\n"

        "void main()\n"
        "{\n"
        "    // Normalized pixel coordinates (from 0 to 1)\n"
        "    vec2 uv = (gl_FragCoord.xy-iViewport.xy)/iResolution.xy;\n"
        "    vec3 col = loudColor;\n"
        "    float mask = clamp(sign(iLevel - uv.x),0.0,1.0);\n"
        "    gl_FragColor = vec4(col*mask, 1.0);\n"
        "}\n";

    quad.reset(new Shape(openGLContext));

    std::unique_ptr<OpenGLShaderProgram> newShader(new OpenGLShaderProgram(openGLContext));

    if (newShader->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        && newShader->addFragmentShader("#version 410\n"
            "out " JUCE_MEDIUMP " vec4 fragColor;\n"
            + String(fragmentShader).replace("varying", "in")
                                    .replace("texture2D", "texture")
                                    .replace("gl_FragColor", "fragColor"))
        && newShader->link())
    {
        attributes.reset();
        uniforms.reset();

        shaderProgram.reset(newShader.release());
        shaderProgram->use();

        attributes.reset(new Attributes(openGLContext, *shaderProgram));
        uniforms.reset(new Uniforms(openGLContext, *shaderProgram));
    }
    else
    {
        DBG(newShader->getLastError());
    }
}

void SliderWithMeter::renderOpenGL()
{
    render();
}

void SliderWithMeter::mouseDown(const MouseEvent& e)
{
    Slider::mouseDown(e);
    // https://forum.juce.com/t/fr-make-the-bubble-component-used-for-the-slider-popup-display-customizable/33823/3
    if (auto* popupDisplay = getCurrentPopupDisplay())
        popupDisplay->setComponentEffect(nullptr);
}

void SliderWithMeter::mouseEnter(const MouseEvent& e)
{
    Slider::mouseEnter(e);
    // https://forum.juce.com/t/fr-make-the-bubble-component-used-for-the-slider-popup-display-customizable/33823/3
    if (auto* popupDisplay = getCurrentPopupDisplay())
        popupDisplay->setComponentEffect(nullptr);
}

void SliderWithMeter::mouseMove(const MouseEvent& e)
{
    Slider::mouseMove(e);
    // https://forum.juce.com/t/fr-make-the-bubble-component-used-for-the-slider-popup-display-customizable/33823/3
    if (auto* popupDisplay = getCurrentPopupDisplay())
        popupDisplay->setComponentEffect(nullptr);
}

//==============================================================================

SliderWithMeter::Uniforms::Uniforms(OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram)
{
    iResolution.reset(createUniform(openGLContext, shaderProgram, "iResolution"));
    iViewport.reset(createUniform(openGLContext, shaderProgram, "iViewport"));
    iLevel.reset(createUniform(openGLContext, shaderProgram, "iLevel"));
}

OpenGLShaderProgram::Uniform* SliderWithMeter::Uniforms::createUniform(OpenGLContext& openGLContext,
                                                                  OpenGLShaderProgram&
                                                                  shaderProgram,
                                                                  const char* uniformName)
{
    if (openGLContext.extensions.glGetUniformLocation(shaderProgram.getProgramID(), uniformName) < 0)
        return nullptr;

    return new OpenGLShaderProgram::Uniform(shaderProgram, uniformName);
}