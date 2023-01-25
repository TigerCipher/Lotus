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
            if (DataContext == null)
            {
                e.Cancel = true;
                Application.Current.MainWindow.Hide();
                OpenProjectBrowserDialog();
                if(DataContext != null)
                {
                    Application.Current.MainWindow.Show();
                }
            }
            else
            {
                Closing -= OnMainWindowClosing;
                Project.Current?.Unload();
                DataContext = null;
            }
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

            if (!devModeEnabled && (enginePath == null || !Directory.Exists(Path.Combine(enginePath, @"Lotus\src\Lotus\EngineApi"))))
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
                LotusEngineSrcPath = devModeEnabled ? @$"{LotusPath}\Lotus\src" : @$"{enginePath}\include";
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
