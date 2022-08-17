using System.Windows;
using System.Windows.Controls;

namespace LotusEditor.GameProject
{
    /// <summary>
    /// Interaction logic for NewProjectView.xaml
    /// </summary>
    public partial class NewProjectView : UserControl
    {
        public NewProjectView()
        {
            InitializeComponent();
        }

        private void Create_ButtonBase_OnClick(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as NewProject;
            var projPath = vm.CreateProject(templateListBox.SelectedItem as ProjectTemplate);
            bool dialogResult = false;
            var win = Window.GetWindow(this);

            if (!string.IsNullOrEmpty(projPath))
            {
                dialogResult = true;
                var proj = OpenProject.Open(new ProjectData() { ProjectName = vm.ProjectName, ProjectPath = projPath });
                win.DataContext = proj;
            }

            win.DialogResult = dialogResult;
            win.Close();
        }
    }
}
