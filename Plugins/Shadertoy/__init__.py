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
            if os.path.exists(chrome_path):
                return chrome_path
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
    pass_code = shadertoy_pass[1]['node'].GetShaderToyCode()
    js = f"""
    (() => {{
        var cm = document.querySelector('.CodeMirror').CodeMirror;
        if(cm) {{
            cm.setValue({repr(pass_code)});
        }}
    }})()
    """
    page.evaluate(js)
    
    channel_to_css = {
        'iChannel0': '#texture0',
        'iChannel1': '#texture1', 
        'iChannel2': '#texture2',
        'iChannel3': '#texture3'
    }
    pass_name_to_css = {
        'Buffer A': '#miscAssetThumnail4',
        'Buffer B': '#miscAssetThumnail5',
        'Buffer C': '#miscAssetThumnail6',
        'Buffer D': '#miscAssetThumnail7',
    }
    for item_name, item_value in shadertoy_pass[1].items():
        if item_name == 'node' or item_value is None:
            continue
        if isinstance(item_value, Sh.ShaderToyPassNode):
            page.locator(channel_to_css[item_name]).click()
            node_pass_name = None
            for pass_name, pass_data in shadertoy_context.items():
                if pass_data is not None and pass_data.get('node') == item_value:
                    node_pass_name = pass_name
                    break
            page.locator(pass_name_to_css[node_pass_name]).click()
            page.locator("#pickTextureHeader div").nth(1).click()
   
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
            elif isinstance(node, Sh.Texture2dNode):
                pass  

        traverse_node_chain(Sh.Context.PropertyObject.InputPins[0].SourceNode)
        open_shadertoy_with_playwright(shadertoy_context)

class ImportShaderToyMenuEntry(Sh.MenuEntryExt):
    Label = "Import from shadertoy.com"
    TargetMenu = "File"

    def CanExecute(self):
        return True

    def OnExecute(self):
        pass

def Register():
    Sh.RegisterExt(ImportShaderToyMenuEntry)
    Sh.RegisterExt(ExportShaderToyProperty)

def Unregister():
    Sh.UnregisterExt(ImportShaderToyMenuEntry)
    Sh.UnregisterExt(ExportShaderToyProperty)



