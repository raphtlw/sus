use crate::Shell;

#[derive(Debug, Default, Clone)]
pub struct Window {
    is_minimized: bool,
    id: String,
    pid: String,
}

// impl Default for Window {
//     fn default() -> Self {
//         Self {
//             is_minimized: false,
//             id: String::new(),
//             pid: String::new(),
//         }
//     }
// }

#[allow(dead_code)]
impl Window {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn with_id(&mut self, id: String) -> &mut Self {
        self.id = id;
        self
    }

    pub fn with_is_minimized(&mut self, minimized: bool) -> &mut Self {
        self.is_minimized = minimized;
        self
    }

    pub fn is_minimized(&self) -> bool {
        self.is_minimized
    }

    pub fn id(&self) -> String {
        self.id.clone()
    }

    pub fn pid(&self) -> String {
        self.pid.clone()
    }

    pub fn build(&mut self) -> &Self {
        let xprop_output = Shell::cmd(&format!("xprop -id {}", &self.id));

        let window_state = xprop_output
            .split("\n")
            .collect::<Vec<_>>()
            .into_iter()
            .filter(|x| x.contains("window state"));
        if window_state.clone().count() == 0 {
            return self;
        }
        let window_state = String::from(window_state.collect::<Vec<_>>()[0]);
        let window_state = window_state.replace("window state:", "");
        let window_state = window_state.trim();

        log::debug!("window_state: {}", window_state);

        match window_state {
            "Normal" => self.is_minimized = false,
            "Iconic" => self.is_minimized = true,
            _ => unreachable!(),
        };

        let process_id = xprop_output
            .split("\n")
            .collect::<Vec<_>>()
            .into_iter()
            .filter(|x| x.contains("_NET_WM_PID"));
        let process_id = String::from(process_id.collect::<Vec<_>>()[0]);
        let process_id = process_id.split("=").collect::<Vec<_>>()[1];
        let process_id = process_id.trim();

        self.pid = String::from(process_id);

        self
    }
}
