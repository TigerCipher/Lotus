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

namespace LotusEditor.Project
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
            var projPath = vm.CreateProject((templateListBox.SelectedItem as ProjectTemplate)!);
            bool dialogResult = false;
            var win = Window.GetWindow(this);

            if (!string.IsNullOrEmpty(projPath))
            {
                dialogResult = true;
            }

            win.DialogResult = dialogResult;
            win.Close();
        }
    }
}
