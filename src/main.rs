extern crate sdl2;

use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::audio::{AudioCallback, AudioSpecDesired};
use std::time::Duration;

struct SquareWave 
{
    phase_inc: f32,
    phase: f32,
    volume: f32
}

impl AudioCallback for SquareWave 
{
    type Channel = f32;

    fn callback(&mut self, out: &mut [f32]) 
    {
        for x in out.iter_mut() {
            *x = if self.phase <= 0.5 
            {
                self.volume
            } 
            else 
            {
                -self.volume
            };
            self.phase = (self.phase + self.phase_inc) % 1.0;
        }
    }
}


pub fn main() 
{
    let sdl_context = sdl2::init().unwrap();
    let video_subsystem = sdl_context.video().unwrap();
    let audio_subsystem = sdl_context.audio().unwrap();

    let window = video_subsystem.window("SDL2 demo", 800, 600).position_centered().build().unwrap();

    let mut canvas = window.into_canvas().build().unwrap();

    let desired_spec = AudioSpecDesired 
    {
        freq: Some(44100),
        channels: Some(2),  
        samples: None      
    };

    let device = audio_subsystem.open_playback(None, &desired_spec, |spec| 
        {
            SquareWave 
            {
                phase_inc: 440.0 / spec.freq as f32,
                phase: 0.0,
                volume: 1.0
            }
        }).unwrap();
    

    canvas.set_draw_color(Color::RGB(0, 255, 255));
    canvas.clear();
    canvas.present();
    let mut event_pump = sdl_context.event_pump().unwrap();
    let mut i = 0;
    'running: loop 
    {
        i = (i + 1) % 255;
        canvas.set_draw_color(Color::RGB(i, 64, 255 - i));
        canvas.clear();
        for event in event_pump.poll_iter() 
        {
            match event 
            {
                Event::Quit {..} | Event::KeyDown{ keycode: Some(Keycode::Escape), .. } => 
                {
                    break 'running
                },
                Event::KeyDown{ keycode: Some(Keycode::LCtrl), .. } =>
                {
                    device.resume();
                },
                _ => 
                {
                    device.pause();
                }
            }
        }

        canvas.present();
        ::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
    }
}