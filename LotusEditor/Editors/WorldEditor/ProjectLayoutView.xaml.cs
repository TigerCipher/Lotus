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
using LotusEditor.Components;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.Editors
{
    /// <summary>
    /// Interaction logic for ProjectLayoutView.xaml
    /// </summary>
    public partial class ProjectLayoutView : UserControl
    {
        public ProjectLayoutView()
        {
            InitializeComponent();
        }

        private void AddEntity_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var btn = sender as Button;
            var scene = btn.DataContext as Scene;
            scene.AddEntityCmd.Execute(new Entity(scene)
            {
                Name = "Empty Entity"
            });
        }

        private void GameEntities_OnListBoxSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var listbox = sender as ListBox;
            var newSelection = listbox.SelectedItems.Cast<Entity>().ToList();
            var prevSelection = newSelection.Except(e.AddedItems.Cast<Entity>())
                .Concat(e.RemovedItems.Cast<Entity>()).ToList();

            Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                "Selection changed",
                () =>
                {
                    listbox.UnselectAll();
                    prevSelection.ForEach(x=> (listbox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                },
                () =>
                {
                    listbox.UnselectAll();
                    newSelection.ForEach(x => (listbox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                }
            ));

            MSGameEntity msEnt = null;
            if (newSelection.Any())
            {
                msEnt = new MSGameEntity(newSelection);
            }

            GameEntityView.Instance.DataContext = msEnt;

        }
    }
}
