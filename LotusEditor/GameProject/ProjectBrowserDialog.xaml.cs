using System.Windows;

namespace LotusEditor.GameProject
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
            if(Equals(sender, createProjectButton))
            {
                if(openProjectButton.IsChecked == true)
                {
                    openProjectButton.IsChecked = false;
                    browserContent.Margin = new Thickness(20);
                }
                createProjectButton.IsChecked = true;
            }else if (Equals(sender, openProjectButton))
            {
                if (createProjectButton.IsChecked == true)
                {
                    createProjectButton.IsChecked = false;
                    browserContent.Margin = new Thickness(-Width + 20, 20,20,20);
                }
                openProjectButton.IsChecked = true;
            }
        }
    }
}
