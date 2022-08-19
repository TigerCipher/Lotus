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
    /// Interaction logic for GameEntityView.xaml
    /// </summary>
    public partial class GameEntityView : UserControl
    {
        public static GameEntityView Instance { get; private set; }

        private Action _undo;
        private string _propertyName;

        public GameEntityView()
        {
            InitializeComponent();
            DataContext = null;
            Instance = this;
            DataContextChanged += (_, __) =>
            {
                if (DataContext != null)
                {
                    (DataContext as MSEntity).PropertyChanged += (s, e) => _propertyName = e.PropertyName;
                }
            };
        }

        private Action GetRenameAction()
        {
            var msEnt = DataContext as MSEntity;
            var selection = msEnt.SelectedEntities.Select(entity => (entity, entity.Name)).ToList();
            return () =>
            {
                selection.ForEach(item => item.entity.Name = item.Name);
                (DataContext as MSEntity).Refresh();
            };
        }

        private void Name_OnTextBoxKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            _undo = GetRenameAction();
        }

        private void Name_OnTextBoxKeyboardFocusLost(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (_propertyName != nameof(MSEntity.Name) || _undo == null) return;
            var redo = GetRenameAction();
            Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction("Rename entity", _undo, redo));
            _undo = null;
        }


        private Action GetEnableAction()
        {
            var msEnt = DataContext as MSEntity;
            var selection = msEnt.SelectedEntities.Select(entity => (entity, entity.IsEnabled)).ToList();
            return () =>
            {
                selection.ForEach(item => item.entity.IsEnabled = item.IsEnabled);
                (DataContext as MSEntity).Refresh();
            };
        }

        private void IsEnabled_OnCheckboxClick(object sender, RoutedEventArgs e)
        {
            var undo = GetEnableAction();
            var msEnt = DataContext as MSEntity;
            msEnt.IsEnabled = (sender as CheckBox).IsChecked;
            var redo = GetEnableAction();
            Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(msEnt.IsEnabled == true ? "Enabled entity" : "Disable entity", undo, redo));
        }
    }
}
