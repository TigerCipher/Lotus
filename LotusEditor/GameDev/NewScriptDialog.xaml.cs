using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using LotusEditor.Utility;
using LotusEditor.GameProject;

namespace LotusEditor.GameDev
{
    /// <summary>
    /// Interaction logic for NewScriptDialog.xaml
    /// </summary>
    public partial class NewScriptDialog : Window
    {

        private static readonly string _cppTemplate = @"// Generated by the Lotus Engine
#include ""{0}.h""

namespace {1}
{{
LOTUS_REGISTER_SCRIPT({0});

void {0}::OnStart()
{{
    // Perform initialization
}}

void {0}::Update(lotus::Timestep ts)
{{
    // Perform logic
}}


}} // namespace {1}
";

        private static readonly string _hTemplate = @"// Generated by the Lotus Engine
#pragma once

namespace {1}
{{

class {0} : public lotus::script::ScriptableEntity
{{
public:
    constexpr explicit {0}(lotus::entity::Entity entity) : lotus::script::ScriptableEntity(entity) {{}}
    void OnStart() override;
    void Update(lotus::Timestep ts) override;
private:
}};

}} // namespace {1}
";

        private static readonly string _namespace = GetNamespaceFromProject();

        private static string GetNamespaceFromProject()
        {
            var projName = Project.Current.Name;
            if (string.IsNullOrEmpty(projName)) return string.Empty;
            projName = StringUtil.ReplaceWhitespace(projName, "_");
            return projName;
        }

        public NewScriptDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
            scriptPathTextBox.Text = @"Scripts\";
        }


        bool Validate()
        {
            var valid = false;

            var name = StringUtil.ReplaceWhitespace(scriptNameTextBox.Text.Trim(), "");
            var path = scriptPathTextBox.Text.Trim();
            var errMsg = string.Empty;
            var cppName = Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Location, path),
                $"{name}.cpp"));
            var hName = Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Location, path),
                $"{name}.h"));

            if (string.IsNullOrEmpty(name)) errMsg = "Give the script a name";
            else if (name.IndexOfAny(Path.GetInvalidFileNameChars()) != -1 || name.Any(char.IsWhiteSpace))
                errMsg = "Invalid characters in the script name";
            else if (string.IsNullOrEmpty(path)) errMsg = "Select a valid folder for your scripts";
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
                errMsg = "Invalid characters in the script path";
            else if (!Path.GetFullPath(Path.Combine(Project.Current.Location, path))
                         .Contains(Path.Combine(Project.Current.Location, @"Scripts\")))
                errMsg = "Script must be added to the projects Scripts folder, or a sub folder";
            else if (File.Exists(cppName) || File.Exists(hName))
                errMsg = $"Script {name} already exists";
            else valid = true;

            if (!valid)
                msgTextBlock.Foreground = FindResource("Editor.RedBrush") as Brush;
            else msgTextBlock.Foreground = FindResource("Editor.FontColorBrush") as Brush;

            msgTextBlock.Text = errMsg;

            return valid;
        }

        private void ScriptName_OnTextChanged(object sender, TextChangedEventArgs e)
        {
            if (!Validate()) return;
            var name = StringUtil.ReplaceWhitespace(scriptNameTextBox.Text.Trim(), "");
            var proj = Project.Current;
            msgTextBlock.Text = $"{name}.h & {name}.cpp will be added to {Project.Current.Name}";
        }

        private void ScriptPath_OnTextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private async void Create_OnButtonClick(object sender, RoutedEventArgs e)
        {
            if (!Validate()) return;
            IsEnabled = false;
            busyAnimation.Opacity = 0;
            busyAnimation.Visibility = Visibility.Visible;
            var fadeIn = new DoubleAnimation(0, 1, new Duration(TimeSpan.FromMilliseconds(500)));
            busyAnimation.BeginAnimation(OpacityProperty, fadeIn);

            try
            {
                var name = StringUtil.ReplaceWhitespace(scriptNameTextBox.Text.Trim(), "");
                var path = Path.GetFullPath(Path.Combine(Project.Current.Location, scriptPathTextBox.Text.Trim()));
                var solution = Project.Current.SolutionName;
                var projName = Project.Current.Name;
                Logger.Info($"Attemping to create script {name} at {path} for Visual Studio solution {solution} for Project {projName}");
                await Task.Run(() => CreateScript(name, path, solution, projName));
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Error($"Failed to create script {scriptNameTextBox.Text}");
            }
            finally
            {
                var fadeOut = new DoubleAnimation(1, 0, new Duration(TimeSpan.FromMilliseconds(200)));
                fadeOut.Completed += (s, e) =>
                {
                    busyAnimation.Opacity = 0;
                    busyAnimation.Visibility = Visibility.Visible;
                    Close();
                };
                busyAnimation.BeginAnimation(OpacityProperty, fadeOut);
            }
        }

        private void CreateScript(string name, string path, string solution, string projName)
        {
            if (!Directory.Exists(path)) Directory.CreateDirectory(path);
            var cppName = Path.GetFullPath(Path.Combine(path, $"{name}.cpp"));
            var hName = Path.GetFullPath(Path.Combine(path, $"{name}.h"));

            using (var sw = File.CreateText(cppName))
            {
                sw.Write(_cppTemplate, name, _namespace);
            }
            using (var sw = File.CreateText(hName))
            {
                sw.Write(_hTemplate, name, _namespace);
            }

            var files = new[] { cppName, hName };

            for (int i = 0; i < 3; ++i)
            {
                if (!VisualStudio.AddFilesToSolution(solution, projName, files))
                    System.Threading.Thread.Sleep(1000);
                else break;
            }
        }
    }
}
