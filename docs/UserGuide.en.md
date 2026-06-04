# ShaderHelper User Guide

ShaderHelper uses the shader code editor to implement shading logic, and uses the node editor to build and visualize render pipeline dependencies. It currently provides two main authoring workflows: one for users who enjoy the Shadertoy workflow, and one for users who prefer writing native HLSL/GLSL freely.

<img src="../ScreenShot/Doc1.png" alt="Workflow overview" width="800">

## Shadertoy Workflow

This workflow is designed for users who are familiar with or prefer Shadertoy-style authoring.

1. Create a `ShaderToy graph` asset and a `Shadertoy shader` asset.
2. Double-click the `Shadertoy shader` asset to open it, then build the pipeline and edit the shader.

<img src="../ScreenShot/Doc2.gif" alt="Create Shadertoy assets" width="800">

![Edit Shadertoy pipeline](../ScreenShot/Doc9.png)

### Importing and Exporting shadertoy.com Shaders

To import from or export to shadertoy.com, first enable the `Shadertoy` plugin in `Preferences`.

![Enable Shadertoy plugin](../ScreenShot/Doc3.png)

After the plugin is enabled, right-click in the `AssetBrowser` panel and choose `Import from shadertoy.com` to import a shader. ShaderHelper will automatically create the corresponding assets after the import succeeds.

![Import from shadertoy.com](../ScreenShot/Doc4.png)

You can also export directly to shadertoy.com by selecting the `Present` node in a Shadertoy graph.

<img src="../ScreenShot/Doc5.png" alt="Export to shadertoy.com" width="800">

Note: shadertoy.com does not allow external assets. If you want to share a shader on shadertoy.com, the shader should only depend on built-in Shadertoy assets.

## Native HLSL/GLSL Workflow

This workflow is designed for users who want to write native HLSL/GLSL directly and organize their own render pipeline.

1. Create `Render`, `Shader`, and `Material` assets.
2. Write shader code in the `Shader` asset. Saving the file or clicking the button in the lower-left area can trigger compilation.
3. A `Shader` asset can contain multiple stages. Use the selector in the lower-right area to choose the language service context for the current stage.

   <img src="../ScreenShot/Doc20.png" alt="Compile shader" width="800">

   <img src="../ScreenShot/Doc6.png" alt="Shader stage" width="800">

4. Configure render state and initial binding data in the `Material` asset.
5. Bindings can use built-in data. Right-click a binding item to switch the built-in binding source.

<img src="../ScreenShot/Doc7.png" alt="Material render state and bindings" width="800">

![Switch builtin bindings](../ScreenShot/Doc8.png)

### Preview Rendering and Custom Rendering

The native HLSL/GLSL workflow provides two modes: `Preview Rendering` and `Custom Rendering`.

![Rendering modes](../ScreenShot/Doc10.png)

`Preview Rendering` is used to adjust the scene objects required for rendering. `Custom Rendering` renders according to the configuration in the `Render` graph asset.

When a `Camera` scene object is set, it affects the computed built-in data, such as `Model`, `ViewProj`, `CameraPos`, and other values.

<img src="../ScreenShot/Doc11.gif" alt="Custom rendering camera data" width="800">

You can also set `Preview Camera` to change the preview camera.

<img src="../ScreenShot/Doc12.gif" alt="Preview camera" width="800">

## Print and Assert

ShaderHelper supports `Print` and `Assert` in shaders to help inspect runtime results and locate problems.

```hlsl
Print("{0},{1},{2}", t1, t2, t3); // Currently supports up to 3 arguments
PrintAtMouse("{0}", t);           // Prints the result at the mouse position; only valid in Shadertoy shaders

Assert(uv.x > 0.9);
Assert(uv.x > 0.9, "uv.x must be greater than 0.9");
AssertFormat(uv.x > 0.9, "uv.x {0} must be greater than 0.9", uv.x);
```

`Print` and `Assert` results can be viewed in the log.

<img src="../ScreenShot/Doc13.gif" alt="Print result" width="800">

<img src="../ScreenShot/Doc14.gif" alt="Assert result" width="800">

When an `Assert` condition is not satisfied, the corresponding vertex, pixel, or thread is highlighted in pink during debugging.

<img src="../ScreenShot/Doc17.png" alt="Assert highlight" width="800">

## Shader Debugging

ShaderHelper supports single-step debugging for vertex shaders, pixel shaders, and compute shaders. The debugger does not simulate shaders through a CPU virtual machine. Instead, it instruments the shader and reads results back from the GPU, which can be more accurate than virtual-machine simulation in some cases.

1. Select the `RenderObject` of a `MeshPass` node in a `Render` graph to debug Vertex or Pixel shaders.
2. Select a `ComputePass` node in a `Render` graph to debug Compute shaders.

   <img src="../ScreenShot/Doc18.gif" alt="Render graph shader debugging" width="800">

3. Select a `ShaderPass` node in a `Shadertoy` graph to debug Pixel shaders.

   <img src="../ScreenShot/Doc15.gif" alt="Shadertoy shader debugging" width="800">

## Pixel Shader Global Inspection

When single-step debugging a pixel shader, you can globally inspect each line's result. This feature supports loops and included header files.

<img src="../ScreenShot/Doc16.gif" alt="Pixel shader global inspection" width="800">

## UBSan Validation

ShaderHelper can help validate potential shader issues. Enable `UBSan` in `Preferences` -> `Environment` to use this feature.

After `UBSan` is enabled, ShaderHelper reports errors when potential issues are encountered during shader debugging. To validate the entire shader globally, click the validation button in the toolbar.

Note: enabling `UBSan` can reduce debugging performance.

<img src="../ScreenShot/Doc19.png" alt="UBSan validation" width="800">
