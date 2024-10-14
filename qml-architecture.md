### Audacity's

```mermaid
classDiagram
  AppWindow *-- AppTitleBar
  AppWindow *-- WindowContent
  DockWindow <|-- WindowContent
  WindowContent *-- InteractiveProvider
  WindowContent *-- NavigationSection
  WindowContent *-- DockToolBar
  WindowContent *-- HomePage
  WindowContent *-- NotationPage

  namespace toolBars {
    class DockToolBar
  }

  namespace pages {
    class HomePage
    class NotationPage
  }
```
