import sys
import os
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
    if shadertoy_pass[0] in options:
        select_element.select_option(label=shadertoy_pass[0])
    page.locator("#passManager label", has_text=shadertoy_pass[0]).first.click()
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

def open_shadertoy_with_playwright(shadertoy_context):
    global _playwright_context, _playwright_instance
    browser_path = get_browser_path()
    if not browser_path:
        print("Could not find the browser, only support chromium based browser.")
        return

    try:
        if _playwright_instance:
            _playwright_instance.stop()
            _playwright_instance = None

        _playwright_instance = sync_playwright().start()
        user_data_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".shadertoy_browser_profile")
        _playwright_context = _playwright_instance.chromium.launch_persistent_context(
            user_data_dir,
            executable_path=browser_path,
            headless=False,
        )
        
        page = _playwright_context.pages[0]
        url = "https://www.shadertoy.com/new"
        page.goto(url)
        for pass_name, pass_data in shadertoy_context.items():
            if pass_data is not None:
                init_shadertoy_pass(page, shadertoy_context, (pass_name, pass_data))

        page.locator("#passManager label", has_text='Image').first.click()
        page.locator("#compileButton").click()

    except Exception as e:
        print(f"launch browser failed: {e}")

class ExportShaderToyProperty(Sh.PropertyExt):
    TargetType = Sh.ShaderToyOutputNode

    def CreateWidget(self):
        button = Sh.Slate.Button()
        button.Text = "Export to shadertoy.com"
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

class ImportShaderToyMenuEntry(Sh.MenuEntryExt):
    Label = "Import from shadertoy.com"
    TargetMenu = "File"

    def CanExecute(self):
        return False

    def OnExecute(self):
        pass

class ImportShaderToyResourcesMenuEntry(Sh.MenuEntryExt):
    Label = "Import shadertoy.com resources"
    TargetMenu = "Asset"

    def CanExecute(self):
        return True

    def OnExecute(self):
        shadertoy_resource_dir = os.path.join(Sh.PathHelper.ResourceDir, "ShaderToy")
        for root, dirs, files in os.walk(shadertoy_resource_dir):
            for file in files:
                file_path = os.path.join(root, file)
                importer = Sh.Asset.GetImporter(file_path)
                if importer:
                    asset = importer.CreateAsset(file_path)
                    asset_ext = asset.FileExtension
                    file_without_ext, ext = os.path.splitext(file)
                    saved_file_path = os.path.join(Sh.Context.CurAssetBrowserDir, f"{file_without_ext}.{asset_ext}")
                    Sh.Asset.SaveToFile(asset, saved_file_path)

def Register():
    Sh.RegisterExt(ImportShaderToyMenuEntry)
    Sh.RegisterExt(ExportShaderToyProperty)
    Sh.RegisterExt(ImportShaderToyResourcesMenuEntry)

def Unregister():
    Sh.UnregisterExt(ImportShaderToyMenuEntry)
    Sh.UnregisterExt(ExportShaderToyProperty)
    Sh.UnregisterExt(ImportShaderToyResourcesMenuEntry)



