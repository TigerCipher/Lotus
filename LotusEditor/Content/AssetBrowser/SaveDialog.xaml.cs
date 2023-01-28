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

namespace LotusEditor.Content
{
    /// <summary>
    /// Interaction logic for SaveDialog.xaml
    /// </summary>
    public partial class SaveDialog : Window
    {

        public string SaveFilePath { get; private set; }
        public SaveDialog()
        {
            InitializeComponent();
        }

        private void Save_OnButtonClick(object sender, RoutedEventArgs e)
        {
            if (ValidateFileName(out var saveFilePath))
            {
                SaveFilePath = saveFilePath;
                DialogResult = true;
                Close();
            }
        }

        private bool ValidateFileName(out string saveFilePath)
        {
            var assetBrowser = assetBrowserView.DataContext as AssetBrowser;
            var path = assetBrowser.SelectedFolder;
            if (!Path.EndsInDirectorySeparator(path)) path += @"\";
            var filename = filenameTextBox.Text.Trim();
            if (string.IsNullOrEmpty(filename))
            {
                saveFilePath = string.Empty;
                return false;
            }

            if (!filename.EndsWith(Asset.AssetFileExtension))
                filename += Asset.AssetFileExtension;

            path += $@"{filename}";
            var isValid = false;
            var errorMsg = string.Empty;

            if (filename.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
            {
                errorMsg = "Invalid Character(s) used in file name";
            }
            else if (File.Exists(path) && MessageBox.Show("File already exists. Overwrite?", "Overwrite File", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.No)
            {
                saveFilePath = string.Empty;
                return false;
            }
            else
            {
                isValid = true;
            }

            if (!string.IsNullOrEmpty(errorMsg))
            {
                MessageBox.Show(errorMsg, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }

            saveFilePath = path;
            return isValid;
        }

        private void AssetBrowser_OnDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if ((e.OriginalSource as FrameworkElement)?.DataContext == assetBrowserView.SelectedItem &&
                assetBrowserView.SelectedItem?.FileName == filenameTextBox.Text)
            {
                Save_OnButtonClick(sender, null);
            }
        }
    }
}
