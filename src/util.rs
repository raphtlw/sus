use std::process::Command;

pub enum Shell {}

impl Shell {
    pub fn cmd<'a>(cmd: &'a str) -> String {
        let output = Command::new("sh")
            .arg("-c")
            .arg(cmd)
            .output()
            .expect("Failed to run shell command");

        String::from_utf8_lossy(&output.stdout).into_owned()
    }
}
