using System;
using System.Collections.Generic;
using System.ComponentModel;
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
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using LotusEditor.GameProject;

namespace LotusEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainWindowLoaded;
            Closing += OnMainWindowClosing;
        }

        public static string LotusPath { get; private set; } = @"D:\CppWorkspace\Lotus";
        public static string LotusEngineSrcPath { get; private set; } = @"D:\CppWorkspace\Lotus\Lotus\src";

        private void OnMainWindowClosing(object sender, CancelEventArgs e)
        {
            Closing -= OnMainWindowClosing;
            Project.Current?.Unload();
        }

        private void OnMainWindowLoaded(object sender, RoutedEventArgs evt)
        {
            Loaded -= OnMainWindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }

        private void GetEnginePath()
        {
            var enginePath = Environment.GetEnvironmentVariable("LOTUS_ENGINE", EnvironmentVariableTarget.User);
            var devMode =
                        Environment.GetEnvironmentVariable("LOTUS_DEVELOPER_MODE", EnvironmentVariableTarget.User);

            var devModeEnabled = devMode != null && devMode == "TRUE";

            if (!devModeEnabled && (enginePath == null || !Directory.Exists(Path.Combine(enginePath, @"Lotus\EngineApi"))))
            {
                var dlg = new EnginePathDialog();
                if (dlg.ShowDialog() == true)
                {
                    LotusPath = dlg.LotusPath;
                    Environment.SetEnvironmentVariable("LOTUS_ENGINE", LotusPath.ToUpper(), EnvironmentVariableTarget.User);
                }
                else
                {
                    Application.Current.Shutdown();
                }
            }
            else
            {
                LotusPath = devModeEnabled ? @"D:\CppWorkspace\Lotus" : enginePath;
            }
        }

        private void OpenProjectBrowserDialog()
        {
            var projBrowser = new ProjectBrowserDialog();
            if (projBrowser.ShowDialog() == false || projBrowser.DataContext == null)
            {
                Application.Current.Shutdown();
                return;
            }

            Project.Current?.Unload();
            DataContext = projBrowser.DataContext;
        }

    }
}
