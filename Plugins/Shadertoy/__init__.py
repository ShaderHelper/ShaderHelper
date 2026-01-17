import sys
import os
import re
import json
from playwright.sync_api import sync_playwright
import ShaderHelper as Sh

_playwright_context = None
_playwright_instance = None

def get_browser_path():
    try:
        if sys.platform.startswith('win'):
            import winreg
            with winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\Shell\Associations\UrlAssociations\http\UserChoice"
            ) as key:
                prog_id = winreg.QueryValueEx(key, "ProgId")[0]
            with winreg.OpenKey(
                winreg.HKEY_CLASSES_ROOT,
                fr"{prog_id}\shell\open\command"
            ) as key:
                cmd = winreg.QueryValueEx(key, None)[0]
                if cmd.startswith('"'):
                    path = cmd.split('"')[1]
                else:
                    path = cmd.split(' ')[0]
                return path
        elif sys.platform.startswith('darwin'):
            chrome_path = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
            edge_path = "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge"
            if os.path.exists(chrome_path):
                return chrome_path
            elif os.path.exists(edge_path):
                return edge_path
            return None      
    except Exception as e:
        return None
        
def init_shadertoy_pass(page, shadertoy_context, shadertoy_pass):
    select_element = page.locator('select.tabAddSelect')
    options = select_element.locator('option').all_text_contents()
    shadertoy_pass_name = shadertoy_pass[0]
    if shadertoy_pass_name in options:
        select_element.select_option(label=shadertoy_pass_name)
    page.locator("#passManager label", has_text=shadertoy_pass_name).first.click()
    page.wait_for_selector('.CodeMirror')
    shadertoy_pass_node = shadertoy_pass[1]['node']
    pass_code = shadertoy_pass_node.GetShaderToyCode()
    js = f"""
    (() => {{
        var cm = document.querySelector('.CodeMirror').CodeMirror;
        if(cm) {{
            cm.setValue({repr(pass_code)});
        }}
    }})()
    """
    page.evaluate(js)

    pass_name_to_css = {
        'Buffer A': '#miscAssetThumnail4',
        'Buffer B': '#miscAssetThumnail5',
        'Buffer C': '#miscAssetThumnail6',
        'Buffer D': '#miscAssetThumnail7',
    }
    pagebutton_to_texture = {
        '#pageButton0' : ['Abstract 1', 'Abstract 2', 'Abstract 3', 'Bayer', 'Blue Noise', 'Font 1', 'Gray Noise Medium', 'Gray Noise Small', 'Lichen'],
        '#pageButton1' : ['London', 'Nyancat', 'Organic 1', 'Organic 2', 'Organic 3', 'Organic 4', 'Pebbles', 'RGBA Noise Medium', 'RGBA Noise Small'],
        '#pageButton2' : ['Rock Tiles', 'Rusty Metal', 'Stars', 'Wood'],
    }
    # set channels
    for item_name, item_value in shadertoy_pass[1].items():
        if item_name == 'node' or item_value is None:
            continue
        channel_index = ''.join(filter(str.isdigit, item_name))
        texture_selector = f"#texture{channel_index}"
        resource_set_successfully = False
        if isinstance(item_value, Sh.ShaderToyPassNode):
            page.locator(texture_selector).click()
            node_pass_name = None
            for pass_name, pass_data in shadertoy_context.items():
                if pass_data is not None and pass_data.get('node') == item_value:
                    node_pass_name = pass_name
                    break
            page.locator("//a[@onclick=\"openTab('Misc')\"]").click()
            page.locator(pass_name_to_css[node_pass_name]).click()
            page.locator("#pickTextureHeader div").nth(1).click()
            resource_set_successfully = True
        elif isinstance(item_value, Sh.Texture2dNode):
            texture = item_value.Texture
            if texture is not None:
                is_shadertoy_texture = False
                for pagebutton, texture_list in pagebutton_to_texture.items():
                    if texture.FileName in texture_list:
                        is_shadertoy_texture = True
                        break
                if is_shadertoy_texture:
                    page.locator(texture_selector).click()
                    page.locator("//a[@onclick=\"openTab('Textures')\"]").click()
                    page.locator(pagebutton).click()
                    span = page.locator('span.spanName', has_text=texture.FileName).first
                    td = span.locator('xpath=ancestor::td[1]')
                    img = td.locator('xpath=preceding-sibling::td[1]//img').first
                    img_id = img.get_attribute("id")
                    page.locator(f"#{img_id}").click()
                    page.locator("#pickTextureHeader div").nth(1).click()
                    resource_set_successfully = True
        elif isinstance(item_value, Sh.ShaderToyKeyboardNode):
            page.locator(texture_selector).click()
            page.locator("//a[@onclick=\"openTab('Misc')\"]").click()
            page.locator("#miscAssetThumnail0").click()
            page.locator("#pickTextureHeader div").nth(1).click()
            resource_set_successfully = True
        elif isinstance(item_value, Sh.ShaderToyPreviousFrameNode):
            if shadertoy_pass_name != 'Image' and shadertoy_pass_node is item_value.GetPassNode():
                page.locator(texture_selector).click()
                page.locator("//a[@onclick=\"openTab('Misc')\"]").click()
                page.locator(pass_name_to_css[shadertoy_pass_name]).click()
                page.locator("#pickTextureHeader div").nth(1).click()
                resource_set_successfully = True

        if resource_set_successfully:
            # set sampling setting
            sampling_button_selector = f"#mySamplingButton{channel_index}"
            samplerfilter_selector = f"#mySamplerFilter{channel_index}"
            samplerwrap_selector = f"#mySamplerWrap{channel_index}"
            samplervflip_selector = f"#mySamplerVFlip{channel_index}"
            page.locator(sampling_button_selector).click()
            channel_desc = getattr(shadertoy_pass_node, f"iChannel{channel_index}")
            page.locator(samplerfilter_selector).select_option(channel_desc.Filter.name.lower())
            page.locator(samplerwrap_selector).select_option(channel_desc.Wrap.name.lower())
            if isinstance(item_value, Sh.Texture2dNode):
                page.locator(samplervflip_selector).uncheck()
            page.locator(sampling_button_selector).click()

def launch_playwright():
    global _playwright_context, _playwright_instance
    browser_path = get_browser_path()
    if not browser_path:
        print("Could not find the browser, only support chromium based browser.")
        return

    if _playwright_instance:
        _playwright_instance.stop()
        _playwright_instance = None

    _playwright_instance = sync_playwright().start()
    user_data_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".shadertoy_browser_profile")
    _playwright_context = _playwright_instance.chromium.launch_persistent_context(
        user_data_dir,
        executable_path=browser_path,
        headless=False,
        args=[
            '--disable-blink-features=AutomationControlled',
            '--no-sandbox',
            '--disable-dev-shm-usage',
        ],
    )
        

def open_shadertoy_with_playwright(shadertoy_context):
    try:
        launch_playwright()
        page = _playwright_context.pages[0]
        url = "https://www.shadertoy.com/new"
        page.goto(url)
        for pass_name, pass_data in shadertoy_context.items():
            if pass_data is not None:
                init_shadertoy_pass(page, shadertoy_context, (pass_name, pass_data))

        page.locator("#passManager label", has_text='Image').first.click()
        page.locator("#compileButton").click()
        page.locator("#myPauseButton").click()
        page.locator("#myResetButton").click()
        page.locator("#myPauseButton").click()

    except Exception as e:
        print(f"launch browser failed: {e}")

class ExportShaderToyProperty(Sh.PropertyExt):
    TargetType = Sh.ShaderToyOutputNode

    def CreateWidget(self):
        button = Sh.Slate.Button("Export to shadertoy.com")
        button.HAlign = Sh.Slate.HAlign_Center
        button.VAlign = Sh.Slate.VAlign_Center
        button.OnClicked = self.execute
        return button
    
    def execute(self):
        assert isinstance(Sh.Context.PropertyObject, Sh.ShaderToyOutputNode)

        shadertoy_context = {'Image': None, 'Buffer A': None, 'Buffer B': None, 'Buffer C': None, 'Buffer D': None}
        def traverse_node_chain(node):
            if isinstance(node, Sh.ShaderToyPassNode):
                node_pass = None
                for pass_name in shadertoy_context:
                    if shadertoy_context[pass_name] is None:
                        shadertoy_context[pass_name] = {'node': node}
                        node_pass = pass_name
                        break
                    elif shadertoy_context[pass_name]['node'] == node:
                        node_pass = pass_name
                if node_pass is None:
                    return
                for InputPin in node.InputPins:
                    shadertoy_context[node_pass][InputPin.Name] = InputPin.SourceNode
                    traverse_node_chain(InputPin.SourceNode)

        traverse_node_chain(Sh.Context.PropertyObject.InputPins[0].SourceNode)
        open_shadertoy_with_playwright(shadertoy_context)

def extract_shader_id(url):
    match = re.search(r'/view/([a-zA-Z0-9]+)', url)
    if match:
        return match.group(1)
    return None

def create_shadertoy_assets(shadertoy_id, shadertoy_info):
    if shadertoy_info is not None:
        target_dir = os.path.join(Sh.Context.CurAssetBrowserDir, shadertoy_id)
        os.makedirs(target_dir, exist_ok=True)
        
        shadertoy_name = shadertoy_info['Name']
        shadertoy_graph = Sh.Asset.CreateAsset(shadertoy_name, Sh.ShaderToy)

        id_to_builtin_resource = {
            "XdX3Rn": "Abstract 1",
            "4dX3Rn": "Abstract 2",
            "XdXGzr": "Abstract 3",
            "Xdf3zn": "Bayer",
            "XsBSR3": "Blue Noise",
            "4dXGzr": "Font 1",
            "4dXGzn": "Gray Noise Medium",
            "4sf3Rn": "Gray Noise Small",
            "4sfGRn": "Lichen",
            "4dfGRn": "London",
            "Xsf3Rn": "Nyancat",
            "XsXGRn": "Organic 1",
            "XsX3Rn": "Organic 2",
            "Xsf3Rr": "Organic 3",
            "4df3Rr": "Organic 4",
            "4sf3Rr": "Pebbles",
            "Xsf3zn": "RGBA Noise Medium",
            "XdXGzn": "RGBA Noise Small",
            "4dXGRn": "Rock Tiles",
            "4sXGRn": "Rusty Metal",
            "XdfGRn": "Stars",
            "XsfGRn": "Wood"
        }
    
        id_to_node = {}
        # Create the shadertoy graph that contains all needed nodes and shaders
        for pass_name, pass_data in shadertoy_info['RenderPass'].items():
            if pass_data is not None:
                shadertoy_shader = Sh.Asset.CreateAsset(pass_name, Sh.StShader)
                shadertoy_shader.EditorContent = pass_data['code']
                shadertoy_shader.Language = Sh.GpuShaderLanguage.GLSL
                saved_file_path = os.path.join(target_dir, f"{pass_name}.{shadertoy_shader.FileExtension}")
                Sh.Asset.SaveToFile(shadertoy_shader, saved_file_path)
                
                shadertoy_pass_node = Sh.ShaderToyPassNode(shadertoy_graph, shadertoy_shader)
                pass_id = pass_data['outputs'][0]['id']
                id_to_node[pass_id] = shadertoy_pass_node
                for input_data in pass_data['inputs']:
                    input_id = input_data['id']
                    if input_id in id_to_builtin_resource:
                        if input_data['type'] == 'texture':
                            tex_path = os.path.join(Sh.PathHelper.BuiltinDir, "ShaderToy", f"{id_to_builtin_resource[input_id]}.texture")
                            shadertoy_tex = Sh.Asset.LoadAsset(tex_path)
                            if shadertoy_tex is not None:
                                texture_node = Sh.Texture2dNode(shadertoy_graph, shadertoy_tex)
                                id_to_node[input_id] = texture_node
                                shadertoy_graph.AddNode(texture_node)
                    elif input_data['type'] == 'keyboard':
                        keyboard_node = Sh.ShaderToyKeyboardNode(shadertoy_graph)
                        id_to_node[input_id] = keyboard_node
                        shadertoy_graph.AddNode(keyboard_node)

                    channel_index = input_data['channel']
                    sampler = input_data['sampler']
                    channel_desc = getattr(shadertoy_pass_node, f"iChannel{channel_index}")
                    filter_str = sampler['filter']
                    if filter_str == 'linear':
                        channel_desc.Filter = Sh.ShaderToyFilterMode.Linear
                    elif filter_str == 'nearest':
                        channel_desc.Filter = Sh.ShaderToyFilterMode.Nearest
                
                    wrap_str = sampler['wrap']
                    if wrap_str == 'clamp':
                        channel_desc.Wrap = Sh.ShaderToyWrapMode.Clamp
                    elif wrap_str == 'repeat':
                        channel_desc.Wrap = Sh.ShaderToyWrapMode.Repeat
                
                shadertoy_graph.AddNode(shadertoy_pass_node)

        # Add Links between nodes
        present_node = None
        for node in shadertoy_graph.Nodes:
            if isinstance(node, Sh.ShaderToyOutputNode):
                present_node = node
                break

        for pass_name, pass_data in shadertoy_info['RenderPass'].items():
            if pass_data is not None:
                pass_id = pass_data['outputs'][0]['id']
                pass_node = id_to_node[pass_id]
                if pass_data['name'] == 'Image':
                    shadertoy_graph.AddLink(pass_node.GetPin('RT'), present_node.GetPin('RT'))
                for input_data in pass_data['inputs']:
                    input_id = input_data['id']
                    if input_id in id_to_node:
                        input_node = id_to_node[input_id]
                        channel_index = input_data['channel']
                        # self-dependency
                        if input_node == pass_node:
                            preframe_node = Sh.ShaderToyPreviousFrameNode(shadertoy_graph, pass_node)
                            shadertoy_graph.AddNode(preframe_node)
                            shadertoy_graph.AddLink(preframe_node.GetPin('RT'), pass_node.GetPin(f"iChannel{channel_index}"))
                        else:
                            shadertoy_graph.AddLink(input_node.GetPin('RT'), pass_node.GetPin(f"iChannel{channel_index}"))
               

        saved_file_path = os.path.join(target_dir, f"{shadertoy_name}.{shadertoy_graph.FileExtension}")
        Sh.Asset.SaveToFile(shadertoy_graph, saved_file_path)

def scrape_shadertoy_shader(shader_url):
    global _playwright_context, _playwright_instance
    try:
        launch_playwright()
        page = _playwright_context.pages[0]
        page.goto(shader_url)

        api_response = None
        def check_response(response):
            return (response.url == 'https://www.shadertoy.com/shadertoy' and response.request.method == 'POST')
        with page.expect_response(check_response,timeout=30000) as response_info:
            api_response = response_info.value

        response_text = api_response.text()
        shader_data = json.loads(response_text)[0]

        shadertoy_info = {}
        shadertoy_info['RenderPass'] = {'Image': None, 'Buffer A': None, 'Buffer B': None, 'Buffer C': None, 'Buffer D': None}
        shadertoy_info['Name'] = shader_data['info']['name']
        for pass_name in shadertoy_info['RenderPass']:
            for pass_data in shader_data['renderpass']:
                if pass_data.get('name') == pass_name:
                    shadertoy_info['RenderPass'][pass_name] = pass_data
                    break
        return shadertoy_info

    except Exception as e:
        print(f"Failed to scrape shadertoy shader: {e}")
        return None
    finally:
        if _playwright_context:
            _playwright_context.close()
            _playwright_context = None
        if _playwright_instance:
            _playwright_instance.stop()
            _playwright_instance = None
class ImportShaderToyMenuEntry(Sh.MenuEntryExt):
    Label = "Import from shadertoy.com"
    TargetMenu = "Asset"

    def CanExecute(self):
        return True

    def OnExecute(self):
        window = Sh.Slate.Window("Shadertoy Import")
        window.Size = Sh.Vector2D(400, 60)
        box = Sh.Slate.VBox()

        urlbox = Sh.Slate.HBox()
        urlbox.AddSlot(Sh.Slate.TextBlock("URL: ")).AutoWidth().VAlign(Sh.Slate.VAlign_Center)
        urlTextBox = Sh.Slate.EditableTextBox()
        urlbox.AddSlot(urlTextBox)
        box.AddSlot(urlbox).AutoHeight().Padding(0, 4, 0, 4)

        buttonBox = Sh.Slate.HBox()
        importButton = Sh.Slate.Button("Import")

        errorText = Sh.Slate.TextBlock()
        errorText.ColorAndOpacity = Sh.LinearColor(1, 0, 0, 1)
        
        def on_import_clicked():
            url = urlTextBox.GetText()
            if (shadertoy_id := extract_shader_id(url)) is not None:
                try:
                    shadertoy_info = scrape_shadertoy_shader(url)
                    create_shadertoy_assets(shadertoy_id, shadertoy_info)
                    Sh.Slate.DestroyWindow(window)
                except Exception as e:
                    print(f"Failed to import: {e}")
                    errorText.Text = "Failed to import"
            else:
                errorText.Text = "Invalid URL"
        
        importButton.OnClicked = on_import_clicked
        cancelButton = Sh.Slate.Button("Cancel")
        cancelButton.OnClicked = lambda: Sh.Slate.DestroyWindow(window)
        buttonBox.AddSlot(errorText).HAlign(Sh.Slate.HAlign_Left).VAlign(Sh.Slate.VAlign_Center)
        buttonBox.AddSlot(importButton).AutoWidth().Padding(0, 0, 4, 0)
        buttonBox.AddSlot(cancelButton).AutoWidth()
        box.AddSlot(buttonBox).AutoHeight()

        window.Content = box
        Sh.Slate.AddWindow(window, Sh.Context.MainWindow)

# class ImportShaderToyResourcesMenuEntry(Sh.MenuEntryExt):
#     Label = "Import shadertoy.com resources"
#     TargetMenu = "Asset"

#     def CanExecute(self):
#         return True

#     def OnExecute(self):
#         shadertoy_resource_dir = os.path.join(Sh.PathHelper.ResourceDir, "ShaderToy")
#         for root, dirs, files in os.walk(shadertoy_resource_dir):
#             for file in files:
#                 file_path = os.path.join(root, file)
#                 importer = Sh.Asset.GetImporter(file_path)
#                 if importer:
#                     asset = importer.CreateAsset(file_path)
#                     asset_ext = asset.FileExtension
#                     file_without_ext, ext = os.path.splitext(file)
#                     saved_file_path = os.path.join(Sh.Context.CurAssetBrowserDir, f"{file_without_ext}.{asset_ext}")
#                     Sh.Asset.SaveToFile(asset, saved_file_path)

def Register():
    Sh.RegisterExt(ImportShaderToyMenuEntry)
    Sh.RegisterExt(ExportShaderToyProperty)
    #Sh.RegisterExt(ImportShaderToyResourcesMenuEntry)

def Unregister():
    Sh.UnregisterExt(ImportShaderToyMenuEntry)
    Sh.UnregisterExt(ExportShaderToyProperty)
    #Sh.UnregisterExt(ImportShaderToyResourcesMenuEntry)



