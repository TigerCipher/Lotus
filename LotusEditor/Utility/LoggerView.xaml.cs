using System;
using System.Collections.Generic;
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
using System.Windows.Shapes;

namespace LotusEditor.Utility
{
    /// <summary>
    /// Interaction logic for LoggerView.xaml
    /// </summary>
    public partial class LoggerView : UserControl
    {
        public LoggerView()
        {
            InitializeComponent();
        }

        private void Clear_OnButtonClick(object sender, RoutedEventArgs e)
        {
            Logger.Clear();
        }

        private void MessageFilter_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var filter = 0x0;
            if (toggleInfo.IsChecked == true) filter |= (int)Level.Info;
            if (toggleWarn.IsChecked == true) filter |= (int)Level.Warning;
            if (toggleError.IsChecked == true) filter |= (int)Level.Error;
            Logger.SetMessageFilter(filter);
        }

        private void OnTextChanged(object sender, DataTransferEventArgs e)
        {
            // TODO Doesn't seem to always work
            scrollViewer.ScrollToBottom();
        }
    }
}
