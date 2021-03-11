mod util;
mod window;

use util::*;
use window::*;

use clap::{App, Arg};
use std::{
    env,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
    thread,
    time::Duration,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = App::new("Sus")
        .arg(
            Arg::with_name("verbose")
                .short("v")
                .help("Enables verbose logging"),
        )
        .get_matches();

    if args.is_present("verbose") {
        env::set_var("RUST_LOG", "trace");
        log::info!("Using verbose mode");
    } else {
        env::set_var("RUST_LOG", "warn");
    }

    pretty_env_logger::init();

    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();

    ctrlc::set_handler(move || {
        r.store(false, Ordering::SeqCst);
    })
    .expect("Error setting Ctrl-C handler");
    log::info!("Waiting for Ctrl-C...");

    while running.load(Ordering::SeqCst) {
        let all_windows = Shell::cmd("wmctrl -l");
        log::debug!("all_windows: {}", all_windows);

        for window_data in all_windows.trim().split("\n").into_iter() {
            let window_id = window_data.split(" ").collect::<Vec<_>>()[0];
            let mut window = Window::new();

            let window = window.with_id(String::from(window_id)).build();

            log::debug!("window_id: {}", window_id);

            let window_is_minimized = window.is_minimized();
            log::debug!("window_is_minimized: {}", window_is_minimized);

            if window_is_minimized {
                // Send SIGSTOP to process
                Shell::cmd(&format!("kill -STOP {}", window.pid()));
            } else {
                // Send SIGCONT to process
                Shell::cmd(&format!("kill -CONT {}", window.pid()));
            }
        }

        thread::sleep(Duration::from_millis(10));
    }

    log::info!("Ctrl-C Recieved, Exiting...");

    // Return to normalcy
    // Resume all processes
    let all_windows = Shell::cmd("wmctrl -l");
    log::debug!("all_windows: {}", all_windows);

    for window_data in all_windows.trim().split("\n").into_iter() {
        let window_id = window_data.split(" ").collect::<Vec<_>>()[0];
        let mut window = Window::new();
        let window = window.with_id(String::from(window_id)).build();

        log::debug!("window_id: {}", window_id);

        // Send SIGCONT to process
        Shell::cmd(&format!("kill -CONT {}", window.pid()));
    }

    log::info!("Process cleanup complete");

    Ok(())
}
