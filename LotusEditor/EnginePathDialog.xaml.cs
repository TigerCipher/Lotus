using System;
using System.Collections.Generic;
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
using LotusEditor.Utility;

namespace LotusEditor
{
    /// <summary>
    /// Interaction logic for EnginePathDialog.xaml
    /// </summary>
    public partial class EnginePathDialog : Window
    {

        public string LotusPath { get; private set; }

        public EnginePathDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
        }

        private void Okay_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var path = pathTextBox.Text.Trim();
            msgTextBlock.Text = string.Empty;

            if (string.IsNullOrEmpty(path))
            {
                msgTextBlock.Text = "Invalid path";
            }else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                msgTextBlock.Text = "Illegal characters used";
            }else if (!Directory.Exists(Path.Combine(path, @"Lotus\src\Lotus\API\")))
            {
                msgTextBlock.Text = "Unable to find the Lotus Engine at the given path";
            }

            if (!string.IsNullOrEmpty(msgTextBlock.Text)) return;

            if (!Path.EndsInDirectorySeparator(path)) path += @"\";
            LotusPath = path;
            Logger.Info("Successfully found the Lotus Engine");
            DialogResult = true;
            Close();
        }
    }
}
