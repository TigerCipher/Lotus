using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace LotusEditor.GameProject
{
    /// <summary>
    /// Interaction logic for OpenProjectView.xaml
    /// </summary>
    public partial class OpenProjectView : UserControl
    {
        public OpenProjectView()
        {
            InitializeComponent();
        }

        private void Open_Button_Click(object sender, RoutedEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OnListBoxItem_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OpenSelectedProject()
        {
            var project = OpenProject.Open(projectsListBox.SelectedItem as ProjectData);
            bool dialogResult = false;
            var win = Window.GetWindow(this);

            if (project != null)
            {
                dialogResult = true;
                win.DataContext = project;
            }

            win.DialogResult = dialogResult;
            win.Close();
        }


    }
}
