﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.237
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Revsoft.Wabbitcode.Properties {
    
    
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "10.0.0.0")]
    internal sealed partial class Editor : global::System.Configuration.ApplicationSettingsBase {
        
        private static Editor defaultInstance = ((Editor)(global::System.Configuration.ApplicationSettingsBase.Synchronized(new Editor())));
        
        public static Editor Default {
            get {
                return defaultInstance;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("Consolas")]
        public global::System.Windows.Media.FontFamily Font {
            get {
                return ((global::System.Windows.Media.FontFamily)(this["Font"]));
            }
            set {
                this["Font"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool ShowLineNumbers {
            get {
                return ((bool)(this["ShowLineNumbers"]));
            }
            set {
                this["ShowLineNumbers"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("False")]
        public bool WordWrap {
            get {
                return ((bool)(this["WordWrap"]));
            }
            set {
                this["WordWrap"] = value;
            }
        }
    }
}
