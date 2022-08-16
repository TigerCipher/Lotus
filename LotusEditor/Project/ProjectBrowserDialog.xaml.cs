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
using System.Windows.Shapes;

namespace LotusEditor.Project
{
    /// <summary>
    /// Interaction logic for ProjectBrowserDialog.xaml
    /// </summary>
    public partial class ProjectBrowserDialog : Window
    {
        public ProjectBrowserDialog()
        {
            InitializeComponent();
        }

        private void OnToggleButton_Click(object sender, RoutedEventArgs e)
        {
            if(sender == createProjectButton)
            {
                if(openProjectButton.IsChecked == true)
                {
                    openProjectButton.IsChecked = false;
                    browserContent.Margin = new Thickness(0);
                }
                createProjectButton.IsChecked = true;
            }else if (sender == openProjectButton)
            {
                if (createProjectButton.IsChecked == true)
                {
                    createProjectButton.IsChecked = false;
                    browserContent.Margin = new Thickness(-Width, 0, 0, 0);
                }
                openProjectButton.IsChecked = true;
            }
        }
    }
}
